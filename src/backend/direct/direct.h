#ifndef _XENODON_BACKEND_DIRECT_DIRECT_H
#define _XENODON_BACKEND_DIRECT_DIRECT_H

#include <memory>
#include "backend/direct/DirectDisplay.h"
#include "utility/Span.h"

struct EventDispatcher;

std::unique_ptr<DirectDisplay> make_direct_display(Span<const char*> args, EventDispatcher& dispatcher);

#endif
