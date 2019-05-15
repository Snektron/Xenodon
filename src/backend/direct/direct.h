#ifndef _XENODON_BACKEND_DIRECT_DIRECT_H
#define _XENODON_BACKEND_DIRECT_DIRECT_H

#include <filesystem>
#include <memory>
#include "backend/direct/DirectDisplay.h"

struct EventDispatcher;

std::unique_ptr<DirectDisplay> create_direct_display(EventDispatcher& dispatcher, std::filesystem::path config_path);

#endif
