#ifndef _XENODON_CAMERA_ORBITCAMERACONTROLLER_H
#define _XENODON_CAMERA_ORBITCAMERACONTROLLER_H

#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "math/Quat.h"
#include "math/Vec.h"
#include "backend/Event.h"

class OrbitCameraController: public CameraController {
    Vec3F center;
    QuatF rotation;
    float distance;

public:
    OrbitCameraController();

    void rotate_pitch(float amount);
    void rotate_yaw(float amount);
    void rotate_roll(float amount);
    void zoom(float amount);

    Camera camera() override;
};

#endif
