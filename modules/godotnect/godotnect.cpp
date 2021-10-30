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
			Freenect::FreenectDevice(_ctx, _index) {
		setDepthFormat(FREENECT_DEPTH_11BIT);

		//Initialize holding buffers for depth and video data
		depth_data.resize(freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT).bytes);
		video_data.resize(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes);
	}

	//Handles passing the feed object into the MyFreenectDevice instance. This is so that for a single kinect we can have
	//two streams, one of depth data and one for video data.
	void initForFeed(CameraFeed *p_feed, int p_idx) {
		//Switch on the given idx value. 0 for depth, 1 for video
		switch (p_idx) {
			//depth init, set the feed_depth object and set active to true.
			case 0:
				feed_depth = p_feed;
				depth_active = true;
				break;
			//video init, set the feed_video object and set active to true
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
		//video data comes in asa uint8_t array.
		uint8_t *rgb = static_cast<uint8_t *>(_rgb);
		//get the write pointer to the Godot internal Vector
		uint8_t *w = video_data.ptrw();


		//We need to check if this feed has activated the video stream from the kinect, as well as if the feed object itself
		//registers as active.
		//This prevents the callback from attempting to write to null image objects, as the freenect side of things gets running faster 
		//than the game engine can get to a ready state to instantiate objects.
		if (video_active && feed_video->is_active()) {
			
			//Copy the data over from the buffer			
			memcpy(w, rgb, video_data.size());
			//Create new refcounted image object
			Ref<Image> img;
			//Instantiate the image
			img.instantiate();
			//Create the image, give it the dimensions and the format and the data buffer
			img->create(640, 480, false, Image::FORMAT_RGB8, video_data);
			//set the feed's RGB stream 
			feed_video->set_RGB_img(img);
		}
	}

	// Do not call directly, even in child
	void DepthCallback(void *_depth, uint32_t timestamp) {
		//Depth data comes in as a uint16_t array from the kinect.
		uint16_t *depth = static_cast<uint16_t *>(_depth);
		//The image object will only accept depth data vectors based on uint8_t.
		uint8_t *w = depth_data.ptrw();

		//We need to check if this feed has activated the depth stream from the kinect, as well as if the feed object itself
		//registers as active.
		//This prevents the callback from attempting to write to null image objects, as the freenect side of things gets running faster 
		//than the game engine can get to a ready state to instantiate objects.
		if (depth_active && feed_depth->is_active()) {
			//testing copy to rgb space
			//Since we're going from a uint16_t to a uint8_t space, we write two locations for the R and G data
			//We technically could also get B data as well, or calculate it, but it won't exist in the inbound stream.
			//TODO: add a calc and value set for the B channel to indicate IR shadow maybe?
			//If there's a way to get the kinect's output to give us that information, we can perhaps set a flag value.
			//chop by two to addres size difference between uint8_t and uint16_t
			for (int i = 0; i < depth_data.size() / 2; i++) {
				w[2 * i + 0] = depth[i] >> 8;
				w[2 * i + 1] = depth[i] & 0xff;
			}

			//New image as dest for depth data
			Ref<Image> img;
			//Need to instantiate the image before creation, as it is a ref object.
			img.instantiate();
			//Create the image, passing resolution and the format, and the depth data.
			img->create(640, 480, false, Image::FORMAT_RG8, depth_data);
			//Set the feed's RGB image to the image created.
			feed_depth->set_RGB_img(img);
		}
	}

	//Create two generic CameraFeed objects to hold pointers back to the two separate feeds we need.
	CameraFeed *feed_depth;
	CameraFeed *feed_video;

private:
	//Godot internal Vector types for depth and video data.
	Vector<uint8_t> depth_data;
	Vector<uint8_t> video_data;

	//Flags to determine whether the feeds have been initialized yet.
	bool depth_active = false;
	bool video_active = false;
};

///////////////////////////////////////////////////////
// CameraFeed implementation

class GodotNectFeed : public CameraFeed {
private:
	MyFreenectDevice *device;

public:
	//The index for this feed, 0 is depth 1 is video
	int feed_idx;
	GodotNectFeed();
	~GodotNectFeed();

	bool activate_feed();

	MyFreenectDevice *get_device() const;

	//Passes the MyFreenectDevice object in, and sets it.
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

//This probably wants to be doing more things? Not sure if this is necessary.
GodotNectFeed::GodotNectFeed() {
	device = nullptr;
};
//Not sure if this needs more cleanup steps here or not.
GodotNectFeed::~GodotNectFeed(){

};

bool GodotNectFeed::activate_feed() {
	//Need to query list of available devices, eventually we might have more than one kinect available

	//For now we can assume that there will only be the one at index 0
	//Pass the feed object into the MyFreenectDevice instance, and give it the required index.
	device->initForFeed(this, this->feed_idx);
	//Switch on the feed index, and start the respective stream from the kinect.
	switch (feed_idx) {
		case 0:
			device->startDepth();
			break;
		case 1:
			device->startVideo();
			break;
		default:
			print_line("This should not happen, wrong feed idx activate");
			break;
	}

	return true;
};

void GodotNectFeed::deactivate_feed() {
	//Need more for clean shutdown handling
	//Switch on feed_idx, stop respective stream from kinect.
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

//This creates and instantiates the feeds presented from the kinect, and manages them with the server.
//This will need overhaul to manage multiple kinect devices, but that will require modifying the libfreenect library itself
//Because currently we cannot get access to low-level serial info from the c++ wrapper side of things.
//Technically, the implementation works and will handle multiple devices, but requires giving it an ID out of the list generated by deviceCount. 
//This is not useful because it doesn't guarantee the ID will be static or give identifying info.
void GodotNect::update_feeds() {
	int num_devices = freenect.deviceCount();

	//For now we just walk the list of devices that exist and initialize each as a feeed.
	for (int i = 0; i < num_devices; i++) {

		//Create depth feed
		Ref<GodotNectFeed> newfeed_depth;
		//Instantiate, as it is a ref object
		newfeed_depth.instantiate();

		//Create and set the device
		newfeed_depth->set_device(&freenect.createDevice<MyFreenectDevice>(i), 0);
		//Give it a friendly name
		newfeed_depth->set_name("Kinect_depth");
		//Add the feed
		add_feed(newfeed_depth);

		//Create video feed
		Ref<GodotNectFeed> newfeed_video;
		//Instantiate, as it is a ref object
		newfeed_video.instantiate();
		//We don't need to create a new MyFreenectDevice here, just pass the one already tied to the newfeed_depth object.
		newfeed_video->set_device(newfeed_depth->get_device(), 1);
		//Give it a friendly name
		newfeed_video->set_name("Kinect_video");
		//Add the feed.
		add_feed(newfeed_video);
	};
};

//Calls update_feeds to add the feeds to the main listing.
GodotNect::GodotNect() {
	update_feeds();
};
//Currently does nothing.
GodotNect::~GodotNect(){

};