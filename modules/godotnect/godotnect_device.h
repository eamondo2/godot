#ifndef GODOTNECT_DEVICE_H
#define GODOTNECT_DEVICE_H

#include "libfreenect.h"
#include "libfreenect.hpp"

#include "core/object/class_db.h"

//=============================================================================
// Device implementation, keeping it here and out of the way for convenience
class GodotNectDevice : public Freenect::FreenectDevice {
public:
	// Testing adding the extra params so that we can pass specific depth/video format options at instantiation time.
	GodotNectDevice(freenect_context *_ctx, int _index, freenect_depth_format depth_fmt, freenect_video_format video_fmt, freenect_resolution resolution_fmt);

	void VideoCallback(void *_rgb, uint32_t timestamp);

	void DepthCallback(void *_depth, uint32_t timestamp);

private:
	Vector<uint16_t> depth_data;
	Vector<uint8_t> video_data;
};

#endif