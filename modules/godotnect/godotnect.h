#ifndef GODOTNECT_H
#define GODOTNECT_H

#include "libfreenect.h"
#include "libfreenect.hpp"

#include "core/object/class_db.h"

//=============================================================================
// GodotNect manager implementation.
// Singleton style, the kinect isn't touched until the command is given to actually initialize and grab the specific device we care about.

class GodotNect : public Object {
	GDCLASS(GodotNect, Object);

	static GodotNect *singleton;

	//Some flag for checking if the kinect has been talked to yet.
	bool kinect_is_initialized;

protected:
	static void _bind_methods();

public:
	static GodotNect *get_singleton();

	GodotNect();

	virtual ~GodotNect();

	//returns the kinect_is_initialized variable from the singleton.
	//want this static so that theoretically you can call without having to try and create a new instance?
	static bool get_kinect_status();

	//Call to initialize the kinect. Returns an error if things don't go well.
	//Perhaps we need to allow for specific device identifiers to be created and passed to this.
	static Error initialize_kinect();

	//Working on ideas for how to get access to the lower-level side of the library below the cpp wrapper.
	//This would allow for actually initializing kinects per serial instead of just a non-static id number.
	//Current trick is still getting hold of the freenect_context object that is needed to pass.
	static struct freenect_device_attributes **fetch_device_listing(freenect_context *ctx);
};

#endif