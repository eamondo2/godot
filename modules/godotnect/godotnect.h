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

public:
  // Some flag for checking if the kinect has been talked to yet.
  bool kinect_is_initialized = false;
  // Bool to indicate whether or not the Freenect object has been constructed
  // yet.
  bool freenect_backend_is_initialized = false;

  // Need a list of GodotNectDevice objects to keep track of.
  // Theoretically we might have multiple of them, assuming that there could be
  // more than one Kinect in use? Though that would be forced to be handled by
  // the hpp version of the library, so it might not be necessary. We're just
  // extending the default constructor parms. in the header we've made, so
  // there's a chance that we don't actually have the ability to snoop quite as
  // much. It would be good to have access to the context variable though, as it
  // would let us list devices, pull serials etc.

  // Struct definition for the map object that stores the devices in use by the
  // instance as a manager.
  struct GodotNectDeviceEntry {
    struct GodotNectDeviceEntry *next; // Pointer to next entry in list.
    GodotNectDevice *device;           // Device entry.
  };

  typedef struct GodotNectDeviceEntry GodotNectDeviceEntry;

  GodotNectDeviceEntry *root_device_entry; // Entry point for linked-list
                                           // organization to store the devices.

  freenect_context *f_ctx_global; // Theoretically gives us access to the
                                  // context held by the manager instance.

  static Freenect::Freenect
      *global_freenect_instance; // Instance of the Freenect object to hang on
                                 // to for managing devices etc.

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

  // Working on ideas for how to get access to the lower-level side of the
  // library below the cpp wrapper. This would allow for actually initializing
  // kinects per serial instead of just a non-static id number. Current trick is
  // still getting hold of the freenect_context object that is needed to pass.
  static struct freenect_device_attributes **
  fetch_device_listing(freenect_context *ctx);
};

#endif