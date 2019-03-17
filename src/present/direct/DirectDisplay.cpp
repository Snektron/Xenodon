#include "present/direct/DirectDisplay.h"

DirectDisplay::DirectDisplay(vk::Instance instance, EventDispatcher& dispatcher, const DisplayConfig& display_config) {
}

vk::Extent2D DirectDisplay::size() const {
    return vk::Extent2D{0, 0};
}

void DirectDisplay::poll_events() {

}
