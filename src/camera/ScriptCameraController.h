#ifndef _XENODON_CAMERA_SCRIPTCAMERACONTROLLER_H
#define _XENODON_CAMERA_SCRIPTCAMERACONTROLLER_H

#include <filesystem>
#include <fstream>
#include "camera/CameraController.h"
#include "camera/Camera.h"
#include "math/Vec.h"

class ScriptCameraController: public CameraController {
    std::ifstream camera_input;
    Camera current_transform;

public:
    ScriptCameraController(const std::filesystem::path& path);

    Camera camera() override;
    bool update(float dt) override;
};

#endif
