#include "godotnect.h"

#include "core/os/mutex.h"
#include "servers/camera/camera_feed.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <mutex>
#include <vector>

class MyFreenectDevice : public Freenect::FreenectDevice {
public:
	MyFreenectDevice(freenect_context *_ctx, int _index) :
			Freenect::FreenectDevice(_ctx, _index),
			m_buffer_video(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes),
			m_buffer_depth(freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT).bytes){
		setDepthFormat(FREENECT_DEPTH_11BIT);

		//Initialize holding buffers for depth and video data
		depth_data.resize(freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT).bytes);
		video_data.resize(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes);
	}

	void initForFeed(CameraFeed *p_feed, int p_idx) {

		switch (p_idx) { 
			case 0:
				feed_depth = p_feed;
				depth_active = true;
				break;
			case 1:
				feed_video = p_feed;
				video_active = true;
				break;
			default:
				break;
		}
	};

	// Do not call directly, even in child
	void VideoCallback(void *_rgb, uint32_t timestamp) {
		uint8_t *rgb = static_cast<uint8_t *>(_rgb);

		uint8_t *w = video_data.ptrw();

		//This will almost certainly not work

		if (video_active && feed_video->is_active()) {
			
			memcpy(w, rgb, getVideoBufferSize());
			
			Ref<Image> img;

			img.instantiate();
			img->create(640, 480, false, Image::FORMAT_RGB8, video_data);

			feed_video->set_RGB_img(img);
		}


	}

	// Do not call directly, even in child
	void DepthCallback(void *_depth, uint32_t timestamp) {
		
		uint16_t *depth = static_cast<uint16_t *>(_depth);

		uint8_t *w = depth_data.ptrw();

		//This also will almost cetainly not work

		//New image as dest for depth data

		if (depth_active && feed_depth->is_active()) {

			
			
			memcpy(w, depth, getDepthBufferSize());
			
			Ref<Image> img;

			img.instantiate();
			img->create(640, 480, false, Image::FORMAT_RG8, depth_data);

			feed_depth->set_RGB_img(img);
		}
	}

	CameraFeed *feed_depth;
	CameraFeed *feed_video;

private:

	std::vector<uint8_t> m_buffer_video;

	std::vector<uint16_t> m_buffer_depth;

	Vector<uint8_t> depth_data;

	Vector<uint8_t> video_data;

	bool depth_active = false;
	bool video_active = false;

};

///////////////////////////////////////////////////////
// CameraFeed implementation

class GodotNectFeed : public CameraFeed {
private:
	MyFreenectDevice *device;
public:
	int feed_idx;
	GodotNectFeed();
	~GodotNectFeed();

	bool activate_feed();

	MyFreenectDevice *get_device() const;

	void set_device(MyFreenectDevice *device, int idx);

	void deactivate_feed();
};

// Return the FreenectDevice object that the feed holds
//Not sure what this will be for yet, probably for using to check if we're already initialized for this device to prevent
//re-init lockup
MyFreenectDevice *GodotNectFeed::get_device() const {
	return device;
};

//Allow for setting the device externally
//idx indicates video or depth info, idx = 0 means depth, idx = 1 means video
void GodotNectFeed::set_device(MyFreenectDevice *p_device, int idx) {

	feed_idx = idx;
	device = p_device;
};

GodotNectFeed::GodotNectFeed() {
	device = nullptr;
};

GodotNectFeed::~GodotNectFeed(){

};

bool GodotNectFeed::activate_feed() {
	//Need to query list of available devices, eventually we might have more than one kinect available

	//For now we can assume that there will only be the one at index 0

	device->initForFeed(this, this->feed_idx);


	switch ( feed_idx ) {
		case 0:
			device->startDepth();
			break;
		case 1:
			device->startVideo();
			break;
		default:
			print_line("This should not happen, wrong feed idx");
			break;
	}

	return true;
};

void GodotNectFeed::deactivate_feed() {
	//Need more for clean shutdown handling
	// device->stopVideo();
	// device->stopDepth();

	switch (feed_idx) {
		case 0:
			device->stopDepth();
			break;
		case 1:
			device->stopVideo();
			break;
		default: 
			print_line("This should not happen, wrong feed idx deactivate");
			break;
	}
};

///////////////////////////////////////////////////////////
// Subclass of CameraServer

void GodotNect::update_feeds() {
	int num_devices = freenect.deviceCount();

	//For now we just walk the list of devices that exist and initialize each as a feeed.
	for (int i = 0; i < num_devices; i++) {
		
		//Create depth feed

		Ref<GodotNectFeed> newfeed_depth;
		
		newfeed_depth.instantiate();

		//Create and set the device
		newfeed_depth->set_device(&freenect.createDevice<MyFreenectDevice>(i), 0);

		newfeed_depth->set_name("Kinect_depth");

		add_feed(newfeed_depth);

		//Create video feed

		Ref<GodotNectFeed> newfeed_video;

		newfeed_video.instantiate();

		newfeed_video->set_device(newfeed_depth->get_device(), 1);

		newfeed_video->set_name("Kinect_video");

		add_feed(newfeed_video);



	};
};

GodotNect::GodotNect() {
	update_feeds();
};

GodotNect::~GodotNect(){

};