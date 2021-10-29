#include "register_types.h"
#include "godotnect.h"


void register_godotnect_types(){

    //cribbed from CameraOSX/CameraWin implementation.
    CameraServer::make_default<GodotNect>();

};

void unregister_godotnect_types(){};