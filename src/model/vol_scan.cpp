#include "model/vol_scan.h"
#include <x86intrin.h>
#include <immintrin.h>

namespace {
    struct LineScanResult {
        uint32_t min;
        uint32_t max;
        uint32_t avg;
    };

    uint32_t line_scan_avx2(uint32_t* ibegin, uint32_t* iend) {
        constexpr const size_t width = 32; // Width in bytes of YMM register

        __m256i min = _mm256_set1_epi32(static_cast<int>(0xFFFFFFFF));
        __m256i max = _mm256_set1_epi32(0);

        uintptr_t begin = reinterpret_cast<uintptr_t>(ibegin);
        uintptr_t end = reinterpret_cast<uintptr_t>(iend);

        if (begin % width != 0) {
            const __m256i* p = reinterpret_cast<const __m256i*>(begin);
            __m256i v = _mm256_castps_si256(_mm256_loadu_ps(reinterpret_cast<const float*>(p)));
            min = _mm256_min_epu8(min, v);
            max = _mm256_max_epu8(max, v);
            begin = begin - begin % width + width; // align to next boundary
        }

        for (; begin < end; begin += width) {
            const __m256i* p = reinterpret_cast<const __m256i*>(begin);
            __m256i v = _mm256_castps_si256(_mm256_load_ps(reinterpret_cast<const float*>(p)));
            min = _mm256_min_epu8(min, v);
            max = _mm256_max_epu8(max, v);
        }

        if (begin != end) {
            const __m256i* p = reinterpret_cast<const __m256i*>(end - width);
            __m256i v = _mm256_castps_si256(_mm256_loadu_ps(reinterpret_cast<const float*>(p)));
            min = _mm256_min_epu8(min, v);
            max = _mm256_max_epu8(max, v);
        }

        __m256i vdiff = _mm256_subs_epu8(max, min);
        __m256i a = _mm256_shuffle_epi32(vdiff, 1);
        __m256i b = _mm256_shuffle_epi32(vdiff, 2);
        __m256i c = _mm256_shuffle_epi32(vdiff, 3);
        a = _mm256_max_epu8(vdiff, a);
        b = _mm256_max_epu8(b, c);
        a = _mm256_max_epu8(a, b);
        b = _mm256_permute2f128_si256(a, b, 1);
        a = _mm256_max_epu8(a, b);

        return static_cast<uint32_t>(_mm_cvtsi128_si32(_mm256_castsi256_si128(a)));
    }
}

VolScanResult vol_scan(uint32_t* pixels, const Vec3Sz& dim, const Aabb& aabb, uint8_t max_diff) {
    return {false, 0};
}
