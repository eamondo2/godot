#include "godotnect_device.h"

// Class constructor def for the device handler
GodotNectDevice::GodotNectDevice(freenect_context *_ctx, int _index, freenect_depth_format depth_fmt, freenect_video_format video_fmt, freenect_resolution resolution_fmt) :
		Freenect::FreenectDevice(_ctx, _index) {
	//Set initial depth format during construction, will expose the setDepthFormat method so that it can be called after the fact as well.
	//The hpp lib file specifies that this will stop and restart the manager/controller when called.
	setDepthFormat(depth_fmt);

	//Initialize holding buffers for depth and video data
	depth_data.resize(freenect_find_depth_mode(resolution_fmt, depth_fmt).bytes);
	video_data.resize(freenect_find_video_mode(resolution_fmt, video_fmt).bytes);
}

//Video callback function
void GodotNectDevice::VideoCallback(void *_rgb, uint32_t timestamp) {
	//video data comes in as a uint8_t array.
	uint8_t *rgb = static_cast<uint8_t *>(_rgb);
}

//Depth data callback function
void GodotNectDevice::DepthCallback(void *_depth, uint32_t timestamp) {
	//Depth data comes in as a uint16_t array from the kinect.
	uint16_t *depth = static_cast<uint16_t *>(_depth);
}
