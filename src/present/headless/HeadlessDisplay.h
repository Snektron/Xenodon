#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "present/Display.h"

class HeadlessDisplay: public Display {
public:
    HeadlessDisplay(std::vector gpu_whitelist);
};

#endif
