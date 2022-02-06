#include "godotnect.h"

#include "core/os/mutex.h"
#include "libusb.h"

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
GodotNect::GodotNect() : m_stop(false) {
  ERR_FAIL_COND(singleton != nullptr); // Test to make sure that the singleton
                                       // is the only instance that exists.
  ERR_FAIL_COND(f_ctx_global !=
                nullptr); // Test to make sure that on creation we don't already
                          // have a context active.

  singleton = this;
}

/**
        Destructor.
**/
GodotNect::~GodotNect() {

  if (get_singleton()->freenect_backend_is_initialized) {
    // We need to call thhe shutdown method here

    // Something hasn't gone well.
    ERR_FAIL_COND(get_singleton()->shutdown_freenect_backend());
  }

  singleton = nullptr;
}

/**
        Fetches the singleton instance.
        @return GodotNect object, the singleton.
**/
GodotNect *GodotNect::get_singleton() { return singleton; }

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

  // Here's where we need to decide if I'm actually going to bother using the
  // existing backend stuff from the lib or do most of it myself. I kinda want
  // to actually implement the device handling stuff on my side, and bypass the
  // c++ lib structures, it'll give more access.

  return true;
}

/**
        Attempts to start up the Freenect library backend, including the
freewheeling thread.
        @return bool for indicating success/failure.
**/
bool GodotNect::initialize_freenect_backend() {

  if (freenect_init(&GodotNect::get_singleton()->f_ctx_global, nullptr) < 0) {
    throw std::runtime_error("Cannot initialize freenect library");
    return false;
  }

  freenect_select_subdevices(
      GodotNect::get_singleton()->f_ctx_global,
      static_cast<freenect_device_flags>(FREENECT_DEVICE_MOTOR |
                                         FREENECT_DEVICE_CAMERA));

  if (pthread_create(&GodotNect::get_singleton()->m_thread, nullptr,
                     GodotNect::async_thread,
                     GodotNect::get_singleton()) != 0) {
    throw std::runtime_error("Cannot initialize freenect thread");
    return false;
  }
  GodotNect::get_singleton()->freenect_backend_is_initialized = true;
  return true;
}

bool GodotNect::shutdown_freenect_backend() {
  GodotNect::get_singleton()->m_stop = true;
  for (DeviceMap::iterator it = GodotNect::get_singleton()->m_devices.begin();
       it != GodotNect::get_singleton()->m_devices.end(); ++it) {
    delete it->second;
  }
  pthread_join(GodotNect::get_singleton()->m_thread, nullptr);
  if (freenect_shutdown(GodotNect::get_singleton()->f_ctx_global) < 0) {
    // Error here.
    return false;
  }

  GodotNect::get_singleton()->freenect_backend_is_initialized = false;
  return true;
}

GodotNectDevice &createDevice(std::string _index,
                              freenect_depth_format depth_fmt,
                              freenect_video_format video_fmt,
                              freenect_resolution resolution_fmt) {

  GodotNect::DeviceMap::iterator it =
      GodotNect::get_singleton()->m_devices.find(_index);

  if (it != GodotNect::get_singleton()->m_devices.end()) {
    delete it->second;
  }

  GodotNectDevice *device =
      new GodotNectDevice(GodotNect::get_singleton()->f_ctx_global, _index,
                          depth_fmt, video_fmt, resolution_fmt);
  GodotNect::get_singleton()->m_devices[_index] = device;
  return *device;
}

void GodotNect::deleteDevice(std::string _index) {
  GodotNect::DeviceMap::iterator it =
      GodotNect::get_singleton()->m_devices.find(_index);
  if (it == GodotNect::get_singleton()->m_devices.end()) {
    return;
  }
  delete it->second;
  GodotNect::get_singleton()->m_devices.erase(it);
}

int GodotNect::deviceCount() {
  return freenect_num_devices(GodotNect::get_singleton()->f_ctx_global);
}

void *GodotNect::async_thread(void *user_data) {

  // GodotNect *godotnect = static_cast<GodotNect *>(user_data);

  while (!GodotNect::get_singleton()->m_stop) {
    static timeval timeout = {1, 0};
    int res = freenect_process_events_timeout(
        GodotNect::get_singleton()->f_ctx_global, &timeout);
    if (res < 0) {
      if (res == LIBUSB_ERROR_INTERRUPTED) {
        continue;
      }
      std::stringstream ss;
      ss << "Cannot process events (libusb error code: " << res << ")";
      throw std::runtime_error(ss.str());
    }
  }

  return nullptr;
}

// Need to remember to fill this properly so that the methods exposed can
// actually be called via GDScript.
/**
        Bind methods for use in GDScript.
**/
void GodotNect::_bind_methods() {}

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