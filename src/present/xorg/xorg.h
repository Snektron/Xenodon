#ifndef _XENODON_PRESENT_XORG_XORG_MAIN_H
#define _XENODON_PRESENT_XORG_XORG_MAIN_H

#include <memory>
#include "present/xorg/XorgDisplay.h"
#include "utility/Span.h"

struct EventDispatcher;

std::unique_ptr<XorgDisplay> make_xorg_display(Span<const char*> args, EventDispatcher& dispatcher);

#endif