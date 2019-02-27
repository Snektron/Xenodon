#include "present/bare/BareDisplay.h"

BareDisplay::BareDisplay(vk::Instance instance, EventDispatcher& dispatcher):
    instance(instance), dispatcher(dispatcher) {
}

vk::Extent2D BareDisplay::size() {
    return vk::Extent2D{0, 0};
}

void BareDisplay::poll_events() {

}
