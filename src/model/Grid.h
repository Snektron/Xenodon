#ifndef _XENODON_MODEL_GRID_H
#define _XENODON_MODEL_GRID_H

#include <utility>
#include <memory>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "math/Vec.h"
#include "utility/Span.h"

class Grid {
public:
    using Pixel = uint32_t;

    struct VolScanResult {
        Pixel avg;
        uint8_t max_diff;
    };

private:
    Vec3Sz dim;
    std::unique_ptr<Pixel[]> data;

    Grid(Vec3Sz dim, std::unique_ptr<Pixel[]>&& data):
        dim(dim), data(std::move(data)) {
    }

public:
    Grid(Vec3<size_t> dim);

    static Grid from_tiff(const char* path);

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

    VolScanResult vol_scan(Vec3Sz bmin, Vec3Sz bmax) const;
};

#endif
