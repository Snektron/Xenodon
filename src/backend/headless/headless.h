#ifndef _XENODON_BACKEND_HEADLESS_HEADLESS_H
#define _XENODON_BACKEND_HEADLESS_HEADLESS_H

#include <memory>
#include <string_view>
#include "backend/headless/HeadlessDisplay.h"

struct EventDispatcher;

std::unique_ptr<HeadlessDisplay> create_headless_display(std::filesystem::path config, std::string_view output);

#endif
