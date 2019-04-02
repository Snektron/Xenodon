#include "graphics/core/Queue.h"

Queue::Queue(const Device2& device, uint32_t family_index, uint32_t index):
    queue(device->getQueue(family_index, index)),
    family_index(family_index),
    index(index) {
}
