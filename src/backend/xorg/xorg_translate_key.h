#ifndef _XENODON_PRESENT_XORG_XORG_TRANSLATE_KEY_H
#define _XENODON_PRESENT_XORG_XORG_TRANSLATE_KEY_H

#include <xcb/xcb_keysyms.h>
#include "backend/Key.h"

Key xorg_translate_key(xcb_keysym_t key);

#endif
