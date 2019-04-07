#ifndef _XENODON_BACKEND_RENDERDEVICE_H
#define _XENODON_BACKEND_RENDERDEVICE_H

#include <utility>
#include <cstdint>
#include "graphics/core/Device.h"
#include "graphics/core/Queue.h"
#include "graphics/command/CommandPool.h"
#include "backend/Output.h"
#include "utility/Span.h"

struct RenderDevice {
    Device device;
    Queue graphics_queue;
    size_t outputs;
    CommandPool graphics_command_pool;

    RenderDevice(Device&& device, uint32_t graphics_queue_family, size_t outputs);
};

#endif
