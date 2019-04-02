#ifndef _XENODON_PRESENT_HEADLESS_HEADLESS_H
#define _XENODON_PRESENT_HEADLESS_HEADLESS_H

#include <memory>
#include "backend/headless/HeadlessDisplay.h"
#include "utility/Span.h"

struct EventDispatcher;

std::unique_ptr<HeadlessDisplay> make_headless_display(Span<const char*> args, EventDispatcher& dispatcher);

#endif
