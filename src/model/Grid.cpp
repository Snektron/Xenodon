#include "model/Grid.h"
#include <iostream>
#include <algorithm>
#include <tiffio.h>
#include <x86intrin.h>
#include "core/Error.h"
#include "fmt/format.h"

namespace {
    struct TiffCloser {
        void operator()(TIFF* tiff) const {
            TIFFClose(tiff);
        }
    };

    using TiffPtr = std::unique_ptr<TIFF, TiffCloser>;
}

Grid::Grid(Vec3Sz dim):
    dim(dim), data(std::make_unique<Pixel[]>(this->size())) {

    for (size_t i = 0; i < this->size(); ++i) {
        this->data[i] = {0, 0, 0, 0};
    }
}

Grid Grid::load_tiff(const std::filesystem::path& path) {
    auto tiff = TiffPtr(TIFFOpen(path.c_str(), "r"));
    if (!tiff) {
        throw Error("Failed to open");
    }

    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t depth = 0;

    do {
        uint32_t layer_width, layer_height;
        TIFFGetField(tiff.get(), TIFFTAG_IMAGEWIDTH, &layer_width);
        TIFFGetField(tiff.get(), TIFFTAG_IMAGELENGTH, &layer_height);

        if (layer_width == 0 || layer_height == 0) {
            throw Error("Layer {} has invalid dimensions ({}x{})", depth, layer_width, layer_height);
        } else if (width == 0) {
            width = layer_width;
            height = layer_height;
        } else if (layer_width != width && layer_height != height) {
            throw Error("Dimensions of layer {} differ from previous dimensions ({}x{}, previously {}x{})",
                depth,
                layer_width,
                layer_height,
                width,
                height
            );
        }

        ++depth;
    } while (TIFFReadDirectory(tiff.get()));

    const size_t size = static_cast<size_t>(depth) * width * height;
    auto data = std::make_unique<Pixel[]>(size);

    fmt::print("{}x{}x{} = {} pixels\n", width, height, depth, size);

    size_t layer_stride = width * height;
    for (uint16_t i = 0; i < depth; ++i) {
        TIFFSetDirectory(tiff.get(), i);

        // TIFFReadRGBAImage writes uint32_t's to the raster, which are in the form ABGR. This means
        // that in-memory, their layout is RGBA if the host machine is little-endian, so instead of
        // an expensive copy routine just do a reinterpret cast.
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

Grid::StdDevResult Grid::stddev_scan(Vec3Sz bmin, Vec3Sz bmax) const {
    bmin.x = std::min(this->dim.x, bmin.x);
    bmin.y = std::min(this->dim.y, bmin.y);
    bmin.z = std::min(this->dim.z, bmin.z);

    bmax.x = std::min(this->dim.x, bmax.x);
    bmax.y = std::min(this->dim.y, bmax.y);
    bmax.z = std::min(this->dim.z, bmax.z);

    size_t n = (bmax.x - bmin.x) * (bmax.y - bmin.y) * (bmax.z - bmin.z);
    if (n == 0) {
        return StdDevResult {
            .avg = {0, 0, 0, 0},
            .stddev = 0
        };
    }

    struct {
        size_t r, g, b, a;
    } accum = {0, 0, 0, 0};

    for (size_t z = bmin.z; z < bmax.z; ++z) {
        size_t z_base = z * this->dim.x * this->dim.y;
        for (size_t y = bmin.y; y < bmax.y; ++y) {
            size_t y_base = y * this->dim.x + z_base;
            for (size_t x = bmin.x; x < bmax.x; ++x) {
                const auto pix = this->data[y_base + x];

                accum.r += static_cast<size_t>(pix.r);
                accum.g += static_cast<size_t>(pix.g);
                accum.b += static_cast<size_t>(pix.b);
                accum.a += static_cast<size_t>(pix.a);
            }
        }
    }

    double nd = static_cast<double>(n);

    struct {
        double r, g, b, a;
    } avg = {
        static_cast<double>(accum.r) / nd,
        static_cast<double>(accum.g) / nd,
        static_cast<double>(accum.b) / nd,
        static_cast<double>(accum.a) / nd
    };

    double stddev = 0;

    auto diff = [](uint8_t x, double u) {
        double y = static_cast<double>(x) - u;
        return y * y;
    };

    for (size_t z = bmin.z; z < bmax.z; ++z) {
        size_t z_base = z * this->dim.x * this->dim.y;
        for (size_t y = bmin.y; y < bmax.y; ++y) {
            size_t y_base = y * this->dim.x + z_base;
            for (size_t x = bmin.x; x < bmax.x; ++x) {
                const auto pix = this->data[y_base + x];

                stddev += diff(pix.r, avg.r) + diff(pix.g, avg.g) + diff(pix.b, avg.b) + diff(pix.a, avg.a);
            }
        }
    }

    return {
        .avg = {
            static_cast<uint8_t>(avg.r),
            static_cast<uint8_t>(avg.g),
            static_cast<uint8_t>(avg.b),
            static_cast<uint8_t>(avg.a)
        },
        .stddev = std::sqrt(stddev / nd)
    };
}