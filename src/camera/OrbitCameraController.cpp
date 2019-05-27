#include "camera/OrbitCameraController.h"
#include <algorithm>

OrbitCameraController::OrbitCameraController(EventDispatcher& dispatcher):
    dispatcher(&dispatcher),
    center{0.5f, 0.5f, 0.5f},
    rotation(QuatF::identity()),
    distance(2.f) {

    auto bind_action = [](Action& var) {
        return [&var](Action a) {
            var = a;
        };
    };

    this->dispatcher->bind(Key::A, bind_action(this->inputs.left));
    this->dispatcher->bind(Key::D, bind_action(this->inputs.right));
    this->dispatcher->bind(Key::W, bind_action(this->inputs.up));
    this->dispatcher->bind(Key::S, bind_action(this->inputs.down));
    this->dispatcher->bind(Key::Q, bind_action(this->inputs.roll_left));
    this->dispatcher->bind(Key::E, bind_action(this->inputs.roll_right));
    this->dispatcher->bind(Key::Up, bind_action(this->inputs.zoom_in));
    this->dispatcher->bind(Key::Down, bind_action(this->inputs.zoom_out));
}

OrbitCameraController::~OrbitCameraController() {
    this->dispatcher->bind(Key::A, nullptr);
    this->dispatcher->bind(Key::D, nullptr);
    this->dispatcher->bind(Key::W, nullptr);
    this->dispatcher->bind(Key::S, nullptr);
    this->dispatcher->bind(Key::Q, nullptr);
    this->dispatcher->bind(Key::E, nullptr);
    this->dispatcher->bind(Key::Up, nullptr);
    this->dispatcher->bind(Key::Down, nullptr);
}

void OrbitCameraController::rotate_pitch(float amount) {
    this->rotation *= QuatF::axis_angle(1, 0, 0, amount);
    this->rotation.normalize();
}

void OrbitCameraController::rotate_yaw(float amount) {
    this->rotation *= QuatF::axis_angle(0, 1, 0, amount);
    this->rotation.normalize();
}

void OrbitCameraController::rotate_roll(float amount) {
    this->rotation *= QuatF::axis_angle(0, 0, 1, amount);
    this->rotation.normalize();
}

void OrbitCameraController::zoom(float amount) {
    this->distance = std::max(this->distance + amount, 0.f);
}

Camera OrbitCameraController::camera() {
    const auto forward = this->rotation.forward();

    return Camera {
        .forward = forward,
        .up = this->rotation.up(),
        .translation = center - forward * distance
    };
}

bool OrbitCameraController::update(float dt) {
    const float sensivity = dt;
    const float zoom_sensivity = dt;

    float dyaw = (this->inputs.left == Action::Press ? sensivity : 0.f) + (this->inputs.right == Action::Press ? -sensivity : 0.f);
    float dpitch = (this->inputs.up == Action::Press ? sensivity : 0.f) + (this->inputs.down == Action::Press ? -sensivity : 0.f);
    float droll = (this->inputs.roll_left == Action::Press ? sensivity : 0.f) + (this->inputs.roll_right == Action::Press ? -sensivity : 0.f);
    float dzoom = (this->inputs.zoom_in == Action::Press ? -zoom_sensivity : 0.f) + (this->inputs.zoom_out == Action::Press ? zoom_sensivity : 0.f);

    this->rotate_yaw(dyaw);
    this->rotate_pitch(dpitch);
    this->rotate_roll(droll);
    this->zoom(dzoom);

    return false;
}