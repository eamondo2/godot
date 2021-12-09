#include "godotnect.h"

#include "core/os/mutex.h"

#include <cmath>
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <vector>

// Stolen from navigation_Server_3d.cpp

/**
        Constructor.
**/
GodotNect::GodotNect() {
  ERR_FAIL_COND(singleton != nullptr); // Test to make sure that the singleton
                                       // is the only instance that exists.
  ERR_FAIL_COND(f_ctx_global !=
                nullptr); // Test to make sure that on creation we don't already
                          // have a context active.
  ERR_FAIL_COND(global_freenect_instance !=
                nullptr); // Just to be sure that things aren't being created
                          // before we want them to.
  singleton = this;
}

/**
        Destructor.
**/
GodotNect::~GodotNect() {


  // Need control here to test if the kinect has been acquired, and drop the
  // connection if that is the case. I think this is where that ought to be
  // done, at least call a separate handler method for that.

  if (this->freenect_backend_is_initialized && this->kinect_is_initialized) {
    // Kinect is managed, we need to drop it and tell it to shut down.
    // I think that we can somehow call the destructor function for all the
    // GodotNectDevice objects we've got created.

    // Theoretically this should nuke it?
    // Technically we can get away with just this and not bother with the above
    // loop, but better safe than sorry.

    delete this->global_freenect_instance;

    // I can see about doing more cleanup if it's required, but the above should
    // be sufficient to get the instance to clean up everything.
  }

  // If we don't have the backend or any kinects active, we don't have much
  // cleanup to perform.

  this->freenect_backend_is_initialized = false;
  this->kinect_is_initialized = false;
  
  singleton = nullptr;
}

/**
        Fetches the singleton instance.
        @return GodotNect object, the singleton.
**/
GodotNect *GodotNect::get_singleton() { return singleton; }

// Need to remember to fill this properly so that the methods exposed can
// actually be called via GDScript.
/**
        Bind methods for use in GDScript.
**/
void GodotNect::_bind_methods() {}

/**
        Returns true/false depending on if the kinect can be reached
        @return bool
**/
bool GodotNect::get_kinect_status() {
  return get_singleton()->kinect_is_initialized;
}

/**
        Returns true/false depending on whether the Freenect backend has been
created.
        @return bool
**/
bool GodotNect::get_freenect_backend_status() {
  return get_singleton()->freenect_backend_is_initialized;
}

/**
        Attempts to initialize the kinect itself.
        @return bool for indicating success/failure
**/
bool GodotNect::initialize_kinect() { 

	//Here's where we need to decide if I'm actually going to bother using the existing backend stuff from the lib or do most of it myself.
	//I kinda want to actually implement the device handling stuff on my side, and bypass the c++ lib structures, it'll give more access.

	return true; 
}

/**
        Attempts to start up the Freenect library backend, including the
freewheeling thread.
        @return bool for indicating success/failure.
**/
bool GodotNect::initialize_freenect_backend() {

  try {
    // Create the instance we need to work with. Calling this here means that
    // the background thread will be running assuming this was successful.
    global_freenect_instance = new Freenect::Freenect;
  } catch (std::runtime_error &e) {
    // If we're here, then the backend fucked up somehow.
    // Use Godot's internal error stuff to handle most of it.
    ERR_FAIL_V_MSG(false, e.what());
  }

  // Set the flag so we know it has been successfully constructed.
  get_singleton()->freenect_backend_is_initialized = true;

  return true;
}

// See
// if (Engine::get_singleton()->is_editor_hint()) {
// 		update(); //will just be drawn
// 		return;
// 	}
// The above bit will let me tell if the current thing being run is inside the
// editor or not, can use this to allow for both in-editor video previews to
// function as well as views that work when the game is actually running.

// I am again thinking of shooting for something like a semaphore such that the
// editor can initialize the kinect, and if the game is run from the editor, the
// instance checks to see if there is an existing handler for the kinect before
// trying to make one. Theoretically it can then be able to grab that instance
// via shared memory or something.

// There's a trick for checking whether or not the instance is running spawned
// from the editor or not.
// OS::has_feature("editor") will return true if the game is running in tools
// mode. See core/os.cpp for has_feature.