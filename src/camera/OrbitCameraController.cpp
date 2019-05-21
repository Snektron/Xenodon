#include "camera/OrbitCameraController.h"
#include <algorithm>

OrbitCameraController::OrbitCameraController():
    center{1.5f, 1.5f, 1.5f},
    rotation(QuatF::axis_angle(0, 1, 0, -1.57f)),
    distance(2.f) {
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
