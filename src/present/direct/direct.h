#ifndef _XENODON_PRESENT_DIRECT_DIRECT_MAIN_H
#define _XENODON_PRESENT_DIRECT_DIRECT_MAIN_H

#include <memory>
#include "present/direct/DirectDisplay.h"
#include "utility/Span.h"

struct EventDispatcher;

std::unique_ptr<DirectDisplay> make_direct_display(Span<const char*> args, EventDispatcher& dispatcher);

#endif
