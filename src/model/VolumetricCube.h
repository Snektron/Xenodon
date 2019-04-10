#ifndef _XENODON_MODEL_VOLUMETRICCUBE_H
#define _XENODON_MODEL_VOLUMETRICCUBE_H

#include <utility>
#include <memory>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "math/Vec.h"
#include "utility/Span.h"

class VolumetricCube {
public:

    using Pixel = uint32_t;
private:
    Vec3Sz dim;
    std::unique_ptr<Pixel[]> data;

    VolumetricCube(Vec3Sz dim, std::unique_ptr<Pixel[]>&& data):
        dim(dim), data(std::move(data)) {
    }

public:
    VolumetricCube(Vec3<size_t> dim);

    static VolumetricCube from_tiff(const char* path);

    Vec3Sz dimensions() const {
        return this->dim;
    }

    size_t size() const {
        return this->dim.x * this->dim.y * this->dim.z;
    }

    Pixel at(Vec3Sz index) const {
        return this->data[index.x + index.y * this->dim.x + index.z * this->dim.x * this->dim.y];
    }

    Span<const Pixel> pixels() const {
        return Span<const Pixel>(this->size(), this->data.get());
    }

    Pixel max_diff(Vec3Sz bmin, Vec3Sz bmax) const;
};

#endif
