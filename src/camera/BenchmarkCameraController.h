#ifndef _XENODON_CAMERA_BENCHMARKCAMERACONTROLLER_H
#define _XENODON_CAMERA_BENCHMARKCAMERACONTROLLER_H

#include <cstddef>
#include "camera/Camera.h"
#include "camera/CameraController.h"
#include "math/Vec.h"
#include "math/Quat.h"

class BenchmarkCameraController: public CameraController {
    Vec3F center;
    QuatF rotation;
    float distance;

    size_t index;
    size_t step;

public:
    BenchmarkCameraController();
    Camera camera() override;
    bool update(float dt) override;
};

#endif
