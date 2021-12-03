#include "godotnect.h"
#include "godotnect_device.h"

#include "core/os/mutex.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <mutex>
#include <vector>

//Stolen from navigation_Server_3d.cpp

GodotNect::GodotNect() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

GodotNect::~GodotNect() {
	singleton = nullptr;

	//Need control here to test if the kinect has been acquired, and drop the connection if that is the case.
	//I think this is where that ought to be done, at least call a separate handler method for that.
}

GodotNect *GodotNect::get_singleton() {
	return singleton;
}

//Need to remember to fill this properly so that the methods exposed can actually be called via GDScript.
void GodotNect::_bind_methods() {
}

/**
Need to allow for hooking via GDScript that will basically just allow you to initialize/destruct a freenect instance.
The problem is that I'm not sure what happens if we just do that as if the whole thing was an object that you can just create one of.

Also, I kind of want to have an overall comms bus so that even if you want to talk to another instance that's already running you don't have
to make a new one.

Something like POSIX semaphores?

I feel like something can be done with shared memory space, and message passing, but that feels like it'd end up being a separate code base from
Godot, and you'd have to start that externally or something.
**/

/**
	Returns true/false depending on if the kinect can be reached
**/
bool GodotNect::get_kinect_status() {
	return false;
}

/**
	Attempts to initialize the kinect itself.
**/
Error GodotNect::initialize_kinect() {
	return OK;
}

// See 
// if (Engine::get_singleton()->is_editor_hint()) {
// 		update(); //will just be drawn
// 		return;
// 	}
// The above bit will let me tell if the current thing being run is inside the editor or not, can use this to allow for both in-editor video previews to function as well as
// views that work when the game is actually running.

// I am again thinking of shooting for something like a semaphore such that the editor can initialize the kinect, and if the game is run from the editor, the instance checks 
// to see if there is an existing handler for the kinect before trying to make one.
// Theoretically it can then be able to grab that instance via shared memory or something.

//There's a trick for checking whether or not the instance is running spawned from the editor or not.
// OS::has_feature("editor") will return true if the game is running in tools mode.
// See core/os.cpp for has_feature.