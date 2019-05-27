#include "camera/BenchmarkCameraController.h"
#include <array>

namespace {
    constexpr const float PI = 3.14159265f;

    struct AnimationStep {
        size_t steps;
        QuatF quat;
        float zoom;
    };

    auto ANIMATION = std::array {
        AnimationStep {
            50,
            QuatF::axis_angle(0, 1, 0, PI / static_cast<float>(50)),
            0
        },
        AnimationStep {
            50,
            QuatF::axis_angle(1, 0, 0, PI / static_cast<float>(50)),
            -0.03f
        },
        AnimationStep {
            50,
            QuatF::axis_angle(0, 1, 0, 2 * PI / static_cast<float>(50)),
            0
        },
    };
}

BenchmarkCameraController::BenchmarkCameraController():
    center{0.5f, 0.5f, 0.5f},
    rotation(QuatF::identity()),
    distance(2.f),
    index(0),
    step(0) {
}

Camera BenchmarkCameraController::camera() {
    const auto forward = this->rotation.forward();

    return Camera {
        .forward = forward,
        .up = this->rotation.up(),
        .translation = center - forward * distance
    };
}

bool BenchmarkCameraController::update(float dt) {
    if (this->index >= ANIMATION.size()) {
        return true;
    }
    ++this->step;

    if (this->step >= ANIMATION[this->index].steps) {
        ++this->index;
        this->step = 0;

        if (this->index >= ANIMATION.size()) {
            return true;
        }
    }

    const auto& animation_step = ANIMATION[this->index];
    this->rotation *= animation_step.quat;
    this->rotation.normalize();
    this->distance += animation_step.zoom;

    return false;
}
