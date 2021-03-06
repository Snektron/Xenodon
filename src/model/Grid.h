#ifndef _XENODON_MODEL_GRID_H
#define _XENODON_MODEL_GRID_H

#include <utility>
#include <memory>
#include <filesystem>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "math/Vec.h"
#include "utility/Span.h"
#include "model/Pixel.h"

class Grid {
public:
    struct VolScanResult {
        Pixel avg;
        uint8_t max_diff;
    };

    struct StdDevResult {
        Pixel avg;
        double stddev;
    };

private:
    Vec3Sz dim;
    std::unique_ptr<Pixel[]> data;

    Grid(Vec3Sz dim, std::unique_ptr<Pixel[]>&& data):
        dim(dim), data(std::move(data)) {
    }

public:
    Grid(Vec3Sz dim);

    static Grid load_tiff(const std::filesystem::path& path);

    VolScanResult vol_scan(Vec3Sz bmin, Vec3Sz bmax) const;

    StdDevResult stddev_scan(Vec3Sz bmin, Vec3Sz bmax) const;

    Vec3Sz dimensions() const {
        return this->dim;
    }

    size_t size() const {
        return this->dim.x * this->dim.y * this->dim.z;
    }

    Pixel at(Vec3Sz index) const {
        return this->data[index.x + index.y * this->dim.x + index.z * this->dim.x * this->dim.y];
    }

    void set(Vec3Sz index, Pixel value) {
        this->data[index.x + index.y * this->dim.x + index.z * this->dim.x * this->dim.y] = value;
    }

    Span<Pixel> pixels() const {
        return Span(this->size(), this->data.get());
    }

    size_t memory_footprint() const {
        return sizeof(Grid) + this->size() * sizeof(Pixel);
    }
};

#endif
