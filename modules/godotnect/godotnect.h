#ifndef GODOTNECT_H
#define GODOTNECT_H

#include "servers/camera_server.h"
#include "thirdparty/libfreenect/libfreenect.hpp"
#include "thirdparty/libfreenect/libfreenect.h"


class GodotNect : public CameraServer {


public: 

    // I think we need to share the context so that we don't run into some issues with sharing usb access?
    //Not sure but here goes nothing.

    Freenect::Freenect freenect; //Shared freenect context for feeds and server.

    GodotNect();
    ~GodotNect();

    void update_feeds();

};

#endif // GODOT_TTS_H