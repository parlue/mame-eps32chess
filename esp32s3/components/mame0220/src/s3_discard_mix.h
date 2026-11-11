#pragma once
#include <cstdint>

// Same end position as the original positive-step loop, including leftover
// samples when playback speed changes. Only one division per sound update.
inline uint32_t s3_discard_mix_remainder(uint32_t first, uint32_t limit, uint32_t step) {
    if (first >= limit) return first - limit;
    const uint32_t remainder = (limit - first) % step;
    return remainder ? step - remainder : 0;
}
