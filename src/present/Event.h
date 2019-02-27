#ifndef _XENODON_PRESENT_EVENT_H
#define _XENODON_PRESENT_EVENT_H

#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include "present/Key.h"

enum class Action {
    Press,
    Release,
    Repeat
};

class EventDispatcher {
    using ActionCallback = std::function<void(Action)>;
    using DigitalCallback = std::function<void(double)>;
    using DisplayClosedCallback = std::function<void()>;

    std::unordered_map<Key, ActionCallback> key_bindings;
    DisplayClosedCallback close_binding = nullptr;

public:
    template <typename F>
    void bind(Key key, F f) {
        this->key_bindings[key] = ActionCallback(f);
    }

    template <typename F>
    void bind_close(F f) {
        this->close_binding = DisplayClosedCallback(f);
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
};

#endif
