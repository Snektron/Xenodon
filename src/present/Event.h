#ifndef _XENODON_PRESENT_EVENT_H
#define _XENODON_PRESENT_EVENT_H

#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include "present/Key.h"

enum class Action {
    Press,
    Release
};

struct EventDispatcher {
    using ActionCallback = std::function<void(Action)>;
    using DigitalCallback = std::function<void(double)>;
    using DisplayClosedCallback = std::function<void()>;
    using ResizeCallback = std::function<void(uint16_t width, uint16_t height)>;

private:
    std::unordered_map<Key, ActionCallback> key_bindings;
    DisplayClosedCallback close_binding = nullptr;
    ResizeCallback resize_binding = nullptr;

public:
    void bind(Key key, ActionCallback f) {
        this->key_bindings[key] = f;
    }

    void bind_close(DisplayClosedCallback f) {
        this->close_binding = f;
    }

    void bind_resize(ResizeCallback f) {
        this->resize_binding = f;
    }

    void dispatch_key_event(Key key, Action action) {
        auto it = this->key_bindings.find(key);
        if (it != this->key_bindings.end())
            it->second(action);
    }

    void dispatch_close_event() {
        if (this->close_binding)
            this->close_binding();
    }

    void dispatch_resize_event(uint16_t width, uint16_t height) {
        if (this->resize_binding)
            this->resize_binding(width, height);
    }
};

#endif
