#ifndef _XENODON_BACKEND_HEADLESS_HEADLESS_H
#define _XENODON_BACKEND_HEADLESS_HEADLESS_H

#include <memory>
#include <filesystem>
#include "backend/headless/HeadlessDisplay.h"

struct EventDispatcher;

std::unique_ptr<HeadlessDisplay> create_headless_display(EventDispatcher& dispatcher, std::filesystem::path config, std::filesystem::path output);

#endif
