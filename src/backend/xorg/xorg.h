#ifndef _XENODON_BACKEND_XORG_XORG_H
#define _XENODON_BACKEND_XORG_XORG_H

#include <memory>
#include <filesystem>
#include "backend/xorg/XorgDisplay.h"

struct EventDispatcher;

std::unique_ptr<XorgDisplay> create_xorg_display(EventDispatcher& dispatcher, std::filesystem::path multi_gpu_config);

#endif
