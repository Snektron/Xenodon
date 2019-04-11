#ifndef _XENODON_MODEL_VOL_SCAN_H
#define _XENODON_MODEL_VOL_SCAN_H

#include <cstdint>
#include "math/Vec.h"

struct VolScanResult {
    bool max_diff_exceeded;
    uint32_t avg;
};

struct Aabb {
    Vec3Sz min, max;
};

VolScanResult vol_scan(uint32_t* pixels, const Vec3Sz& dim, const Aabb& aabb, uint8_t max_diff);

#endif
