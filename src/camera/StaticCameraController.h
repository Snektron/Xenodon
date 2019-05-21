#ifndef _XENODON_CAMERA_STATICCAMERACONTROLLER_H
#define _XENODON_CAMERA_STATICCAMERACONTROLLER_H

#include "camera/CameraController.h"
#include "camera/Camera.h"

class StaticCameraController {
    Camera cam;

public:
    StaticCameraController(const Camera& cam):
        cam(cam) {
    }

    virtual Camera camera() override {
        return this->cam;
    }
};

#endif
