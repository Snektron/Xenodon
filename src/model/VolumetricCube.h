#ifndef _XENODON_MODEL_VOLUMETRICCUBE_H
#define _XENODON_MODEL_VOLUMETRICCUBE_H

#include <utility>
#include <memory>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "math/Vec.h"

class VolumetricCube {
private:
    using Pixel = uint32_t;

    Vec3<size_t> dim;
    std::unique_ptr<Pixel[]> data;

    VolumetricCube(Vec3<size_t> dim, std::unique_ptr<Pixel[]>&& data):
        dim(dim), data(std::move(data)) {
    }

public:
    static VolumetricCube from_tiff(const char* path);

    Vec3<size_t> dimensions() const {
        return this->dim;
    }

    const Pixel* pixels() const {
        return this->data.get();
    }
};

#endif
