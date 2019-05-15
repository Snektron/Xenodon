#ifndef _XENODON_BACKEND_BACKEND_H
#define _XENODON_BACKEND_BACKEND_H

#include <filesystem>
#include <memory>
#include "backend/Display.h"
#include "backend/Event.h"

std::unique_ptr<Display> create_headless_backend(EventDispatcher& dispatcher, std::filesystem::path config, std::filesystem::path output);

std::unique_ptr<Display> create_xorg_backend(EventDispatcher& dispatcher, std::filesystem::path multi_gpu_config);

std::unique_ptr<Display> create_direct_backend(EventDispatcher& dispatcher, std::filesystem::path config);

#endif
