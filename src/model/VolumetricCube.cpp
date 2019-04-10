#include "model/VolumetricCube.h"
#include <iostream>
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

Pixel VolumetricCube::max_diff(Vec3Sz bmin, Vec3Sz bmax) const {
    __m128i min = _mm_cvtsi32_si128(static_cast<int>(0xFFFFFFFF));
    __m128i max = _mm_cvtsi32_si128(0x00000000);

    for (size_t z = bmin.z; z < bmax.z; ++z) {
        size_t z_base = z * this->dim.x * this->dim.y;
        for (size_t y = bmin.y; y < bmax.y; ++y) {
            size_t y_base = y * this->dim.x;
            for (size_t x = bmin.x; x < bmax.x; ++x) {
                size_t index = x + y_base + z_base;
                __m128i pixel = _mm_cvtsi32_si128(static_cast<int>(this->data[index]));
                min = _mm_min_epu8(min, pixel);
                max = _mm_max_epu8(max, pixel);
            }
        }
    }

    __m128i diff = _mm_subs_epu8(max, min);
    return static_cast<uint32_t>(_mm_cvtsi128_si32(diff));
}
