#ifndef _XENODON_BACKEND_EVENT_H
#define _XENODON_BACKEND_EVENT_H

#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include "backend/Key.h"

enum class Action {
    Press,
    Release
};

struct EventDispatcher {
    using ActionCallback = std::function<void(Action)>;
    using DigitalCallback = std::function<void(double)>;
    using DisplayClosedCallback = std::function<void()>;
    using SwapchainRecreateCallback = std::function<void(size_t, size_t)>;
    using MouseMotionCallback = std::function<void(int, int)>;

private:
    std::unordered_map<Key, ActionCallback> key_bindings;
    DisplayClosedCallback close_binding = nullptr;
    SwapchainRecreateCallback swapchain_recreate_binding = nullptr;
    MouseMotionCallback mouse_motion_binding = nullptr;

public:
    void bind(Key key, ActionCallback f) {
        this->key_bindings[key] = f;
    }

    void bind_close(DisplayClosedCallback f) {
        this->close_binding = f;
    }

    void bind_swapchain_recreate(SwapchainRecreateCallback f) {
        this->swapchain_recreate_binding = f;
    }

    void bind_mouse_motion(MouseMotionCallback f) {
        this->mouse_motion_binding = f;
    }

    void dispatch_key_event(Key key, Action action) {
        auto it = this->key_bindings.find(key);
        if (it != this->key_bindings.end()) {
            it->second(action);
        }
    }

    void dispatch_close_event() {
        if (this->close_binding) {
            this->close_binding();
        }
    }

    void dispatch_swapchain_recreate_event(size_t gpu_index, size_t output_index) {
        if (this->swapchain_recreate_binding) {
            this->swapchain_recreate_binding(gpu_index, output_index);
        }
    }

    void dispatch_mouse_motion_event(int dx, int dy) {
        if (this->mouse_motion_binding) {
            this->mouse_motion_binding(dx, dy);
        }
    }
};

#endif
