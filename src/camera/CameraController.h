#ifndef _XENODON_CAMERA_CAMERACONTROLLER_H
#define _XENODON_CAMERA_CAMERACONTROLLER_H

#include "camera/Camera.h"

struct CameraController {
    virtual ~CameraController() = default;
    virtual Camera camera() = 0;
    virtual bool update(float dt) = 0;
};

#endif
