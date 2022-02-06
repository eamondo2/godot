/*************************************************************************/
/*  audio_stream.cpp                                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "audio_stream.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"

void AudioStreamPlayback::start(float p_from_pos) {
	if (GDVIRTUAL_CALL(_start, p_from_pos)) {
		return;
	}
	ERR_FAIL_MSG("AudioStreamPlayback::start unimplemented!");
}
void AudioStreamPlayback::stop() {
	if (GDVIRTUAL_CALL(_stop)) {
		return;
	}
	ERR_FAIL_MSG("AudioStreamPlayback::stop unimplemented!");
}
bool AudioStreamPlayback::is_playing() const {
	bool ret;
	if (GDVIRTUAL_CALL(_is_playing, ret)) {
		return ret;
	}
	ERR_FAIL_V_MSG(false, "AudioStreamPlayback::is_playing unimplemented!");
}

int AudioStreamPlayback::get_loop_count() const {
	int ret;
	if (GDVIRTUAL_CALL(_get_loop_count, ret)) {
		return ret;
	}
	return 0;
}

float AudioStreamPlayback::get_playback_position() const {
	float ret;
	if (GDVIRTUAL_CALL(_get_playback_position, ret)) {
		return ret;
	}
	ERR_FAIL_V_MSG(0, "AudioStreamPlayback::get_playback_position unimplemented!");
}
void AudioStreamPlayback::seek(float p_time) {
	if (GDVIRTUAL_CALL(_seek, p_time)) {
		return;
	}
}

int AudioStreamPlayback::mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) {
	int ret;
	if (GDVIRTUAL_CALL(_mix, p_buffer, p_rate_scale, p_frames, ret)) {
		return ret;
	}
	WARN_PRINT_ONCE("AudioStreamPlayback::mix unimplemented!");
	return 0;
}

void AudioStreamPlayback::_bind_methods() {
	GDVIRTUAL_BIND(_start, "from_pos")
	GDVIRTUAL_BIND(_stop)
	GDVIRTUAL_BIND(_is_playing)
	GDVIRTUAL_BIND(_get_loop_count)
	GDVIRTUAL_BIND(_get_playback_position)
	GDVIRTUAL_BIND(_seek, "position")
	GDVIRTUAL_BIND(_mix, "buffer", "rate_scale", "frames");
}
//////////////////////////////

void AudioStreamPlaybackResampled::_begin_resample() {
	//clear cubic interpolation history
	internal_buffer[0] = AudioFrame(0.0, 0.0);
	internal_buffer[1] = AudioFrame(0.0, 0.0);
	internal_buffer[2] = AudioFrame(0.0, 0.0);
	internal_buffer[3] = AudioFrame(0.0, 0.0);
	//mix buffer
	_mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
	mix_offset = 0;
}

int AudioStreamPlaybackResampled::mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) {
	float target_rate = AudioServer::get_singleton()->get_mix_rate();
	float playback_speed_scale = AudioServer::get_singleton()->get_playback_speed_scale();

	uint64_t mix_increment = uint64_t(((get_stream_sampling_rate() * p_rate_scale * playback_speed_scale) / double(target_rate)) * double(FP_LEN));

	int mixed_frames_total = p_frames;

	for (int i = 0; i < p_frames; i++) {
		uint32_t idx = CUBIC_INTERP_HISTORY + uint32_t(mix_offset >> FP_BITS);
		//standard cubic interpolation (great quality/performance ratio)
		//this used to be moved to a LUT for greater performance, but nowadays CPU speed is generally faster than memory.
		float mu = (mix_offset & FP_MASK) / float(FP_LEN);
		AudioFrame y0 = internal_buffer[idx - 3];
		AudioFrame y1 = internal_buffer[idx - 2];
		AudioFrame y2 = internal_buffer[idx - 1];
		AudioFrame y3 = internal_buffer[idx - 0];

		if (idx <= internal_buffer_end && idx >= internal_buffer_end && mixed_frames_total == p_frames) {
			// The internal buffer ends somewhere in this range, and we haven't yet recorded the number of good frames we have.
			mixed_frames_total = i;
		}

		float mu2 = mu * mu;
		AudioFrame a0 = 3 * y1 - 3 * y2 + y3 - y0;
		AudioFrame a1 = 2 * y0 - 5 * y1 + 4 * y2 - y3;
		AudioFrame a2 = y2 - y0;
		AudioFrame a3 = 2 * y1;

		p_buffer[i] = (a0 * mu * mu2 + a1 * mu2 + a2 * mu + a3) / 2;

		mix_offset += mix_increment;

		while ((mix_offset >> FP_BITS) >= INTERNAL_BUFFER_LEN) {
			internal_buffer[0] = internal_buffer[INTERNAL_BUFFER_LEN + 0];
			internal_buffer[1] = internal_buffer[INTERNAL_BUFFER_LEN + 1];
			internal_buffer[2] = internal_buffer[INTERNAL_BUFFER_LEN + 2];
			internal_buffer[3] = internal_buffer[INTERNAL_BUFFER_LEN + 3];
			if (is_playing()) {
				int mixed_frames = _mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
				if (mixed_frames != INTERNAL_BUFFER_LEN) {
					// internal_buffer[mixed_frames] is the first frame of silence.
					internal_buffer_end = mixed_frames;
				} else {
					// The internal buffer does not contain the first frame of silence.
					internal_buffer_end = -1;
				}
			} else {
				//fill with silence, not playing
				for (int j = 0; j < INTERNAL_BUFFER_LEN; ++j) {
					internal_buffer[j + 4] = AudioFrame(0, 0);
				}
			}
			mix_offset -= (INTERNAL_BUFFER_LEN << FP_BITS);
		}
	}
	return mixed_frames_total;
}

////////////////////////////////

Ref<AudioStreamPlayback> AudioStream::instance_playback() {
	Ref<AudioStreamPlayback> ret;
	if (GDVIRTUAL_CALL(_instance_playback, ret)) {
		return ret;
	}
	ERR_FAIL_V_MSG(Ref<AudioStreamPlayback>(), "Method must be implemented!");
}
String AudioStream::get_stream_name() const {
	String ret;
	if (GDVIRTUAL_CALL(_get_stream_name, ret)) {
		return ret;
	}
	return String();
}

float AudioStream::get_length() const {
	float ret;
	if (GDVIRTUAL_CALL(_get_length, ret)) {
		return ret;
	}
	return 0;
}

bool AudioStream::is_monophonic() const {
	bool ret;
	if (GDVIRTUAL_CALL(_is_monophonic, ret)) {
		return ret;
	}
	return true;
}

void AudioStream::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_length"), &AudioStream::get_length);
	ClassDB::bind_method(D_METHOD("is_monophonic"), &AudioStream::is_monophonic);
	GDVIRTUAL_BIND(_instance_playback);
	GDVIRTUAL_BIND(_get_stream_name);
	GDVIRTUAL_BIND(_get_length);
	GDVIRTUAL_BIND(_is_monophonic);
}

////////////////////////////////

Ref<AudioStreamPlayback> AudioStreamMicrophone::instance_playback() {
	Ref<AudioStreamPlaybackMicrophone> playback;
	playback.instantiate();

	playbacks.insert(playback.ptr());

	playback->microphone = Ref<AudioStreamMicrophone>((AudioStreamMicrophone *)this);
	playback->active = false;

	return playback;
}

String AudioStreamMicrophone::get_stream_name() const {
	//if (audio_stream.is_valid()) {
	//return "Random: " + audio_stream->get_name();
	//}
	return "Microphone";
}

float AudioStreamMicrophone::get_length() const {
	return 0;
}

bool AudioStreamMicrophone::is_monophonic() const {
	return true;
}

void AudioStreamMicrophone::_bind_methods() {
}

AudioStreamMicrophone::AudioStreamMicrophone() {
}

int AudioStreamPlaybackMicrophone::_mix_internal(AudioFrame *p_buffer, int p_frames) {
	AudioDriver::get_singleton()->lock();

	Vector<int32_t> buf = AudioDriver::get_singleton()->get_input_buffer();
	unsigned int input_size = AudioDriver::get_singleton()->get_input_size();
	int mix_rate = AudioDriver::get_singleton()->get_mix_rate();
	unsigned int playback_delay = MIN(((50 * mix_rate) / 1000) * 2, buf.size() >> 1);
#ifdef DEBUG_ENABLED
	unsigned int input_position = AudioDriver::get_singleton()->get_input_position();
#endif

	int mixed_frames = p_frames;

	if (playback_delay > input_size) {
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0.0f, 0.0f);
		}
		input_ofs = 0;
	} else {
		for (int i = 0; i < p_frames; i++) {
			if (input_size > input_ofs && (int)input_ofs < buf.size()) {
				float l = (buf[input_ofs++] >> 16) / 32768.f;
				if ((int)input_ofs >= buf.size()) {
					input_ofs = 0;
				}
				float r = (buf[input_ofs++] >> 16) / 32768.f;
				if ((int)input_ofs >= buf.size()) {
					input_ofs = 0;
				}

				p_buffer[i] = AudioFrame(l, r);
			} else {
				if (mixed_frames == p_frames) {
					mixed_frames = i;
				}
				p_buffer[i] = AudioFrame(0.0f, 0.0f);
			}
		}
	}

#ifdef DEBUG_ENABLED
	if (input_ofs > input_position && (int)(input_ofs - input_position) < (p_frames * 2)) {
		print_verbose(String(get_class_name()) + " buffer underrun: input_position=" + itos(input_position) + " input_ofs=" + itos(input_ofs) + " input_size=" + itos(input_size));
	}
#endif

	AudioDriver::get_singleton()->unlock();

	return mixed_frames;
}

int AudioStreamPlaybackMicrophone::mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) {
	return AudioStreamPlaybackResampled::mix(p_buffer, p_rate_scale, p_frames);
}

float AudioStreamPlaybackMicrophone::get_stream_sampling_rate() {
	return AudioDriver::get_singleton()->get_mix_rate();
}

void AudioStreamPlaybackMicrophone::start(float p_from_pos) {
	if (active) {
		return;
	}

	if (!GLOBAL_GET("audio/driver/enable_input")) {
		WARN_PRINT("Need to enable Project settings > Audio > Enable Audio Input option to use capturing.");
		return;
	}

	input_ofs = 0;

	if (AudioDriver::get_singleton()->capture_start() == OK) {
		active = true;
		_begin_resample();
	}
}

void AudioStreamPlaybackMicrophone::stop() {
	if (active) {
		AudioDriver::get_singleton()->capture_stop();
		active = false;
	}
}

bool AudioStreamPlaybackMicrophone::is_playing() const {
	return active;
}

int AudioStreamPlaybackMicrophone::get_loop_count() const {
	return 0;
}

float AudioStreamPlaybackMicrophone::get_playback_position() const {
	return 0;
}

void AudioStreamPlaybackMicrophone::seek(float p_time) {
	// Can't seek a microphone input
}

AudioStreamPlaybackMicrophone::~AudioStreamPlaybackMicrophone() {
	microphone->playbacks.erase(this);
	stop();
}

AudioStreamPlaybackMicrophone::AudioStreamPlaybackMicrophone() {
}

////////////////////////////////

void AudioStreamRandomPitch::set_audio_stream(const Ref<AudioStream> &p_audio_stream) {
	audio_stream = p_audio_stream;
	if (audio_stream.is_valid()) {
		for (Set<AudioStreamPlaybackRandomPitch *>::Element *E = playbacks.front(); E; E = E->next()) {
			E->get()->playback = audio_stream->instance_playback();
		}
	}
}

Ref<AudioStream> AudioStreamRandomPitch::get_audio_stream() const {
	return audio_stream;
}

void AudioStreamRandomPitch::set_random_pitch(float p_pitch) {
	if (p_pitch < 1) {
		p_pitch = 1;
	}
	random_pitch = p_pitch;
}

float AudioStreamRandomPitch::get_random_pitch() const {
	return random_pitch;
}

Ref<AudioStreamPlayback> AudioStreamRandomPitch::instance_playback() {
	Ref<AudioStreamPlaybackRandomPitch> playback;
	playback.instantiate();
	if (audio_stream.is_valid()) {
		playback->playback = audio_stream->instance_playback();
	}

	playbacks.insert(playback.ptr());
	playback->random_pitch = Ref<AudioStreamRandomPitch>((AudioStreamRandomPitch *)this);
	return playback;
}

String AudioStreamRandomPitch::get_stream_name() const {
	if (audio_stream.is_valid()) {
		return "Random: " + audio_stream->get_name();
	}
	return "RandomPitch";
}

float AudioStreamRandomPitch::get_length() const {
	if (audio_stream.is_valid()) {
		return audio_stream->get_length();
	}

	return 0;
}

bool AudioStreamRandomPitch::is_monophonic() const {
	if (audio_stream.is_valid()) {
		return audio_stream->is_monophonic();
	}

	return true; // It doesn't really matter what we return here, but no sense instancing a many playbacks of a null stream.
}

void AudioStreamRandomPitch::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_audio_stream", "stream"), &AudioStreamRandomPitch::set_audio_stream);
	ClassDB::bind_method(D_METHOD("get_audio_stream"), &AudioStreamRandomPitch::get_audio_stream);

	ClassDB::bind_method(D_METHOD("set_random_pitch", "scale"), &AudioStreamRandomPitch::set_random_pitch);
	ClassDB::bind_method(D_METHOD("get_random_pitch"), &AudioStreamRandomPitch::get_random_pitch);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "audio_stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_audio_stream", "get_audio_stream");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "random_pitch", PROPERTY_HINT_RANGE, "1,16,0.01"), "set_random_pitch", "get_random_pitch");
}

AudioStreamRandomPitch::AudioStreamRandomPitch() {
	random_pitch = 1.1;
}

void AudioStreamPlaybackRandomPitch::start(float p_from_pos) {
	playing = playback;
	float range_from = 1.0 / random_pitch->random_pitch;
	float range_to = random_pitch->random_pitch;

	pitch_scale = range_from + Math::randf() * (range_to - range_from);

	if (playing.is_valid()) {
		playing->start(p_from_pos);
	}
}

void AudioStreamPlaybackRandomPitch::stop() {
	if (playing.is_valid()) {
		playing->stop();
		;
	}
}

bool AudioStreamPlaybackRandomPitch::is_playing() const {
	if (playing.is_valid()) {
		return playing->is_playing();
	}

	return false;
}

int AudioStreamPlaybackRandomPitch::get_loop_count() const {
	if (playing.is_valid()) {
		return playing->get_loop_count();
	}

	return 0;
}

float AudioStreamPlaybackRandomPitch::get_playback_position() const {
	if (playing.is_valid()) {
		return playing->get_playback_position();
	}

	return 0;
}

void AudioStreamPlaybackRandomPitch::seek(float p_time) {
	if (playing.is_valid()) {
		playing->seek(p_time);
	}
}

int AudioStreamPlaybackRandomPitch::mix(AudioFrame *p_buffer, float p_rate_scale, int p_frames) {
	if (playing.is_valid()) {
		return playing->mix(p_buffer, p_rate_scale * pitch_scale, p_frames);
	} else {
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		return p_frames;
	}
}

AudioStreamPlaybackRandomPitch::~AudioStreamPlaybackRandomPitch() {
	random_pitch->playbacks.erase(this);
}
/////////////////////////////////////////////
