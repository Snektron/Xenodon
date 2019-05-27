#ifndef _XENODON_CAMERA_ORBITCAMERACONTROLLER_H
#define _XENODON_CAMERA_ORBITCAMERACONTROLLER_H

#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "math/Quat.h"
#include "math/Vec.h"
#include "backend/Event.h"

class OrbitCameraController: public CameraController {
    EventDispatcher* dispatcher;
    Vec3F center;
    QuatF rotation;
    float distance;

    struct {
        Action left = Action::Release;
        Action right = Action::Release;
        Action up = Action::Release;
        Action down = Action::Release;
        Action roll_left = Action::Release;
        Action roll_right = Action::Release;
        Action zoom_in = Action::Release;
        Action zoom_out = Action::Release;
    } inputs;

public:
    OrbitCameraController(EventDispatcher& dispatcher);
    ~OrbitCameraController();

    void rotate_pitch(float amount);
    void rotate_yaw(float amount);
    void rotate_roll(float amount);
    void zoom(float amount);

    Camera camera() override;
    bool update(float dt) override;
};

#endif
