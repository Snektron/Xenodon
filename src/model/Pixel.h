#ifndef _XENODON_MODEL_PIXEL_H
#define _XENODON_MODEL_PIXEL_H

#include <cstdint>

struct [[gnu::packed]] Pixel {
    uint8_t r, g, b, a;

    constexpr static Pixel unpack(uint32_t packed) {
        return {
            static_cast<uint8_t>(packed & 0xFF),
            static_cast<uint8_t>((packed >> 8) & 0xFF),
            static_cast<uint8_t>((packed >> 16) & 0xFF),
            static_cast<uint8_t>((packed >> 24) & 0xFF)
        };
    }

    constexpr uint32_t pack() const {
        // This should be a no-op on little-endian machines
        return static_cast<uint32_t>(this->r | (this->g << 8) | (this->b << 16) | (this->a << 24));
    }
};

static_assert(sizeof(Pixel) == 4, "Compiler didn't pack Pixel to 4 bytes");

constexpr bool operator==(const Pixel& rhs, const Pixel& lhs) {
    // Since .pack() is basically no-op, this is faster than
    // testing every channel seperately
    return rhs.pack() == lhs.pack();
}

constexpr bool operator!=(const Pixel& rhs, const Pixel& lhs) {
    return !(rhs == lhs);
}

#endif
