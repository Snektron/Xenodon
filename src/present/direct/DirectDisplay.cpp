#include "present/direct/DirectDisplay.h"

DirectDisplay::DirectDisplay(vk::Instance instance, EventDispatcher& dispatcher) {
}

vk::Extent2D DirectDisplay::size() const {
    return vk::Extent2D{0, 0};
}

void DirectDisplay::poll_events() {

}
