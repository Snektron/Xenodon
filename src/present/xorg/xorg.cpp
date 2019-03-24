#include "present/xorg/xorg.h"
#include <array>
#include <vulkan/vulkan.hpp>
#include "present/Event.h"
#include "core/Logger.h"
#include "main_loop.h"

std::unique_ptr<XorgDisplay> make_xorg_display(Span<const char*> args, EventDispatcher& dispatcher) {
    LOGGER.log("Using xorg presenting backend");
    return std::make_unique<XorgDisplay>(dispatcher, 800, 600);
}
