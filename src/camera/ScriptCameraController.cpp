#include "camera/ScriptCameraController.h"
#include "core/Error.h"

ScriptCameraController::ScriptCameraController(const std::filesystem::path& path):
    camera_input(path) {
    if (!this->camera_input) {
        throw Error("Failed to open '{}'", path.native());
    }

    this->update(0);
}

Camera ScriptCameraController::camera() {
    return this->current_transform;
}

bool ScriptCameraController::update(float dt) {
    if (this->camera_input.eof()) {
        return true;
    }

    this->camera_input >> this->current_transform.forward.x;
    this->camera_input >> this->current_transform.forward.y;
    this->camera_input >> this->current_transform.forward.z;

    this->camera_input >> this->current_transform.up.x;
    this->camera_input >> this->current_transform.up.y;
    this->camera_input >> this->current_transform.up.z;

    this->camera_input >> this->current_transform.translation.x;
    this->camera_input >> this->current_transform.translation.y;
    this->camera_input >> this->current_transform.translation.z;

    if (this->camera_input.fail()) {
        throw Error("Syntax error in camera input file");
    }

    this->camera_input >> std::ws; // skip \n

    return false;
}