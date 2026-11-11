// Validate the skipped loop's state against the original loop, including
// changing speeds and leftover larger than the current output interval.
#include "../esp32s3/components/mame0220/src/s3_discard_mix.h"
#include <cassert>
#include <cstdio>

static uint32_t original(uint32_t first, uint32_t limit, uint32_t step) {
    uint32_t sample;
    for (sample = first; sample < limit; sample += step) {}
    return sample - limit;
}
int main() {
    const uint32_t steps[] = {1, 7, 333, 1000, 1500, 10000, 100000};
    for (auto step : steps)
        for (uint32_t first = 0; first < 5000; first += 37)
            for (uint32_t limit = 0; limit < 10000; limit += 113)
                assert(s3_discard_mix_remainder(first, limit, step) == original(first, limit, step));
    uint32_t old = 0, fast = 0;
    for (unsigned i = 0; i < 10000; ++i) {
        const uint32_t limit = ((i * 79) % 300) * 1000;
        const uint32_t step = steps[i % 7];
        old = original(old, limit, step);
        fast = s3_discard_mix_remainder(fast, limit, step);
        assert(old == fast);
    }
    std::puts("PASS: discarded-mix state matches original loops and speed changes");
}
