#ifndef GODOTNECT_H
#define GODOTNECT_H

#include "core/object/class_db.h"
#include "godotnect_device.h"
#include "libfreenect.h"
#include "libfreenect.hpp"

//=============================================================================
// GodotNect manager implementation.
// Singleton style, the kinect isn't touched until the command is given to
// actually initialize and grab the specific device we care about.

class GodotNect : public Object {
  GDCLASS(GodotNect, Object);

  static GodotNect *singleton;

protected:
  static void _bind_methods();

private:
  volatile bool m_stop;
  pthread_t m_thread;

public:
  freenect_context *f_ctx_global; // Theoretically gives us access to the
                                  // context held by the manager instance.

  typedef std::map<std::string, GodotNectDevice *> DeviceMap;

  DeviceMap m_devices;

  // Some flag for checking if the kinect has been talked to yet.
  bool kinect_is_initialized = false;
  // Bool to indicate whether or not the Freenect object has been constructed
  // yet.
  bool freenect_backend_is_initialized = false;

  static GodotNect *get_singleton();

  GodotNect();

  virtual ~GodotNect();

  // returns the kinect_is_initialized variable from the singleton.
  // want this static so that theoretically you can call without having to try
  // and create a new instance?
  static bool get_kinect_status();

  // Returns flag variable indicating whether or not we've constructed the
  // backend for the freenect lib.
  static bool get_freenect_backend_status();

  // Call to initialize the kinect. Returns an error if things don't go well.
  // Perhaps we need to allow for specific device identifiers to be created and
  // passed to this.
  /**
          @return bool, true on success, false otherwise.
  **/
  static bool initialize_kinect();

  /**
          This will create the Freenect::Freenect object that holds/runs the
  background thread for polling/updating devices. It's also what is used to
  create/delete devices.
          @return bool, either true on success or false if something bad has
  happened.
  **/
  static bool initialize_freenect_backend();

  static bool shutdown_freenect_backend();

  static GodotNectDevice createDevice(std::string _index,
                                      freenect_depth_format depth_fmt,
                                      freenect_video_format video_fmt,
                                      freenect_resolution resolution_fmt);

  static void deleteDevice(std::string _index);

  static int deviceCount();

  // Working on ideas for how to get access to the lower-level side of the
  // library below the cpp wrapper. This would allow for actually initializing
  // kinects per serial instead of just a non-static id number. Current trick is
  // still getting hold of the freenect_context object that is needed to pass.
  static struct freenect_device_attributes **
  fetch_device_listing(freenect_context *ctx);

  static void *async_thread(void *user_data);
};

#endif