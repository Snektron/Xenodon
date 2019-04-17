#include "model/Grid.h"
#include <iostream>
#include <algorithm>
#include <tiffio.h>
#include <x86intrin.h>
#include "core/Error.h"
#include "core/Logger.h"

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

Grid::Grid(Vec3Sz dim):
    dim(dim), data(std::make_unique<Pixel[]>(this->size())) {

    for (size_t i = 0; i < this->size(); ++i) {
        this->data[i] = {0, 0, 0, 0};
    }
}

Grid Grid::from_tiff(const char* path) {
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

        // TIFFReadRGBAImage writes uint32_t's to the raster, which are in the form ABGR. This means
        // that in-memory, their layout is RGBA if the host machine is little-endian, so instead of
        // an expensive copy routine just do a reinterpret cast.
        static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "TIFFReadRGBAImage requires little endian");
        TIFFReadRGBAImage(tiff.get(), width, height, reinterpret_cast<uint32_t*>(&data.get()[layer_stride * i]));
    }

    return Grid(
        {width, height, depth},
        std::move(data)
    );
}

Grid::VolScanResult Grid::vol_scan(Vec3Sz bmin, Vec3Sz bmax) const {
    struct {
        size_t r, g, b, a;
    } accum = {0, 0, 0, 0};

    bmin.x = std::min(this->dim.x, bmin.x);
    bmin.y = std::min(this->dim.y, bmin.y);
    bmin.z = std::min(this->dim.z, bmin.z);

    bmax.x = std::min(this->dim.x, bmax.x);
    bmax.y = std::min(this->dim.y, bmax.y);
    bmax.z = std::min(this->dim.z, bmax.z);

    size_t n = (bmax.x - bmin.x) * (bmax.y - bmin.y) * (bmax.z - bmin.z);
    if (n == 0) {
        return VolScanResult {
            .avg = {0, 0, 0, 0},
            .max_diff = 0
        };
    }

    __m128i xmin = _mm_cvtsi32_si128(static_cast<int>(0xFFFFFFFF));
    __m128i xmax = _mm_cvtsi32_si128(0x00000000);

    for (size_t z = bmin.z; z < bmax.z; ++z) {
        size_t z_base = z * this->dim.x * this->dim.y;
        for (size_t y = bmin.y; y < bmax.y; ++y) {
            size_t y_base = y * this->dim.x + z_base;
            for (size_t x = bmin.x; x < bmax.x; ++x) {
                const auto pix = this->data[y_base + x];

                __m128i xpix = _mm_cvtsi32_si128(static_cast<int>(pix.pack()));
                xmin = _mm_min_epu8(xmin, xpix);
                xmax = _mm_max_epu8(xmax, xpix);

                accum.r += static_cast<size_t>(pix.r);
                accum.g += static_cast<size_t>(pix.g);
                accum.b += static_cast<size_t>(pix.b);
                accum.a += static_cast<size_t>(pix.a);
            }
        }
    }

    auto diff = Pixel::unpack(static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_subs_epu8(xmax, xmin))));

    auto avg = Pixel{
        static_cast<uint8_t>(accum.r / n),
        static_cast<uint8_t>(accum.g / n),
        static_cast<uint8_t>(accum.b / n),
        static_cast<uint8_t>(accum.a / n)
    };

    return VolScanResult{
        .avg = avg,
        .max_diff = static_cast<uint8_t>(std::max({diff.r, diff.g, diff.b, diff.a}))
    };
}