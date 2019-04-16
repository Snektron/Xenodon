#include "model/VolumetricCube.h"
#include <iostream>
#include <algorithm>
#include <tiffio.h>
#include <x86intrin.h>
#include "core/Error.h"

using Pixel = VolumetricCube::Pixel;

namespace {
    struct TiffCloser {
        void operator()(TIFF* tiff) const {
            TIFFClose(tiff);
        }
    };

    using TiffPtr = std::unique_ptr<TIFF, TiffCloser>;

    struct TiffError: public Error {
        template <typename... Args>
        TiffError(const char* path, std::string_view fmt, const Args&... args):
            Error(format_error(path, fmt, fmt::make_format_args(args...))) {
        }

    private:
        std::string format_error(const char* path, std::string_view fmt, fmt::format_args args) {
            auto buf = fmt::memory_buffer();
            fmt::format_to(buf, "Error reading file '{}': ", path);
            fmt::vformat_to(buf, fmt, args);
            return fmt::to_string(buf);
        }
    };
}

VolumetricCube::VolumetricCube(Vec3Sz dim):
    dim(dim), data(std::make_unique<Pixel[]>(this->size())) {

    for (size_t i = 0; i < this->size(); ++i) {
        this->data[i] = 0;
    }
}

VolumetricCube VolumetricCube::from_tiff(const char* path) {
    auto tiff = TiffPtr(TIFFOpen(path, "r"));
    if (!tiff) {
        throw Error("Failed to open TIFF file '{}'", path);
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t depth = 0;

    do {
        uint32_t layer_width, layer_height;
        TIFFGetField(tiff.get(), TIFFTAG_IMAGEWIDTH, &layer_width);
        TIFFGetField(tiff.get(), TIFFTAG_IMAGELENGTH, &layer_height);

        if (layer_width == 0 || layer_height == 0) {
            throw TiffError(path, "Layer {} has invalid dimensions ({}x{})", depth, layer_width, layer_height);
        } else if (width == 0) {
            width = layer_width;
            height = layer_height;
        } else if (layer_width != width && layer_height != height) {
            throw TiffError(path, "Dimensions of layer {} differ from previous dimensions ({}x{}, previously {}x{})",
                depth,
                layer_width,
                layer_height,
                width,
                height
            );
        }

        ++depth;
    } while (TIFFReadDirectory(tiff.get()));

    auto data = std::make_unique<Pixel[]>(depth * width * height);

    size_t layer_stride = width * height;
    for (uint16_t i = 0; i < depth; ++i) {
        TIFFSetDirectory(tiff.get(), i);
        TIFFReadRGBAImage(tiff.get(), width, height, data.get() + layer_stride * i);
    }

    return VolumetricCube(
        {width, height, depth},
        std::move(data)
    );
}

VolumetricCube::VolScanResult VolumetricCube::vol_scan(Vec3Sz bmin, Vec3Sz bmax) const {
    union {
        struct {
            uint8_t r, g, b, a;
        } channel;

        Pixel pix;
    } pun;

    struct {
        size_t r, g, b, a;
    } accum = {0, 0, 0, 0};

    struct {
        uint8_t r, g, b, a;
    } min = {0xFF, 0xFF, 0xFF, 0xFF}, max = {0, 0, 0, 0};

    bmin.x = std::min(this->dim.x, bmin.x);
    bmin.y = std::min(this->dim.y, bmin.y);
    bmin.z = std::min(this->dim.z, bmin.z);

    bmax.x = std::min(this->dim.x, bmax.x);
    bmax.y = std::min(this->dim.y, bmax.y);
    bmax.z = std::min(this->dim.z, bmax.z);

    size_t n = (bmax.x - bmin.x) * (bmax.y - bmin.y) * (bmax.z - bmin.z);
    if (n == 0) {
        return VolScanResult {
            0x00000000,
            0x00
        };
    }

    for (size_t z = bmin.z; z < bmax.z; ++z) {
        size_t z_base = z * this->dim.x * this->dim.y;
        for (size_t y = bmin.y; y < bmax.y; ++y) {
            size_t y_base = y * this->dim.x;
            for (size_t x = bmin.x; x < bmax.x; ++x) {
                size_t index = x + y_base + z_base;
                pun.pix = this->data[index];

                min.r = std::min(min.r, pun.channel.r);
                min.g = std::min(min.g, pun.channel.g);
                min.b = std::min(min.b, pun.channel.b);
                min.a = std::min(min.a, pun.channel.a);

                max.r = std::max(max.r, pun.channel.r);
                max.g = std::max(max.g, pun.channel.g);
                max.b = std::max(max.b, pun.channel.b);
                max.a = std::max(max.a, pun.channel.a);

                accum.r += pun.channel.r;
                accum.g += pun.channel.g;
                accum.b += pun.channel.b;
                accum.a += pun.channel.a;
            }
        }
    }

    pun.channel.r = static_cast<uint8_t>(accum.r / n);
    pun.channel.g = static_cast<uint8_t>(accum.g / n);
    pun.channel.b = static_cast<uint8_t>(accum.b / n);
    pun.channel.a = static_cast<uint8_t>(accum.a / n);

    return VolScanResult{
        pun.pix,
        static_cast<uint8_t>(std::max({max.r - min.r, max.g - min.g, max.b - min.b, max.a - min.a}))
    };
}