#include "backend/RenderDevice.h"

RenderDevice::RenderDevice(Device&& device, uint32_t graphics_queue_family, size_t outputs):
    device(std::move(device)),
    graphics_queue(this->device, graphics_queue_family),
    outputs(outputs),
    graphics_command_pool(this->device, this->graphics_queue) {
}
