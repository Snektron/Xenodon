#ifndef _XENODON_BACKEND_RENDERRESOURCES_H
#define _XENODON_BACKEND_RENDERRESOURCES_H

#include <cstdint>
#include "graphics/core/Device.h"
#include "graphics/core/Queue.h"
#include "backend/Output.h"
#include "utility/Span.h"

struct RenderResources {
    Device device;
    Queue graphics_queue;
    size_t outputs;
};

#endif
