#ifndef _XENODON_BACKEND_XORG_XORG_H
#define _XENODON_BACKEND_XORG_XORG_H

#include <memory>
#include "backend/xorg/XorgDisplay.h"
#include "utility/Span.h"

struct EventDispatcher;

std::unique_ptr<XorgDisplay> make_xorg_display(Span<const char*> args, EventDispatcher& dispatcher);

#endif
