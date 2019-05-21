#ifndef _XENODON_CAMERA_CAMERA_H
#define _XENODON_CAMERA_CAMERA_H

#include "math/Vec.h"

struct Camera {
    Vec3F forward;
    Vec3F up;
    Vec3F translation;
};

#endif
