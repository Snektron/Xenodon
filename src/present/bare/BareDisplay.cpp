#include "present/bare/BareDisplay.h"

BareDisplay::BareDisplay(vk::Instance instance, EventDispatcher& dispatcher):
    instance(instance), dispatcher(dispatcher) {
}

void BareDisplay::poll_events() {

}
