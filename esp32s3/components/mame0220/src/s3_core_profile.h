#pragma once
// Diagnostic only. One MAME worker; scopes subtract nested measured work.
#include "esp_timer.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace s3_core_profile {
enum bucket { scheduler, timers, cpu, video, ports, sound, osd, count };
class scope {
    bucket id;
    scope *parent;
    int64_t start, children = 0;
public:
    explicit scope(bucket value);
    ~scope();
    scope(const scope &) = delete;
    scope &operator=(const scope &) = delete;
};
void reset();

#ifdef S3_CORE_PROFILE_IMPLEMENTATION
namespace {
scope *top = nullptr;
int64_t epoch = 0, exclusive[count] = {};
uint32_t calls[count] = {}, sequence = 0, depth = 0, max_depth = 0;
}
void reset() {
    assert(!top && !depth);
    for (unsigned i = 0; i < count; ++i) { exclusive[i] = 0; calls[i] = 0; }
    sequence = max_depth = 0;
    epoch = esp_timer_get_time();
    std::printf("[MAME CORE BEGIN] order=scheduler,timers,cpu,video,ports,sound,osd wall_includes_preemption=1 diagnostic=1\n");
}
scope::scope(bucket value) : id(value), parent(top), start(esp_timer_get_time()) {
    assert(epoch && value < count);
    top = this;
    if (++depth > max_depth) max_depth = depth;
    assert(depth <= 32);
}
scope::~scope() {
    const int64_t end = esp_timer_get_time(), elapsed = end - start;
    assert(top == this && depth && elapsed >= children);
    exclusive[id] += elapsed - children;
    ++calls[id];
    top = parent;
    --depth;
    if (parent) { parent->children += elapsed; return; }
    if (end - epoch < 5000000) return;
    int64_t total = 0;
    for (auto value : exclusive) total += value;
    assert(total <= end - epoch);
    std::printf("[MAME CORE] seq=%u wall_us=%lld exclusive_us=%lld,%lld,%lld,%lld,%lld,%lld,%lld other_us=%lld calls=%u,%u,%u,%u,%u,%u,%u depth=%u\n",
        ++sequence, (long long)(end - epoch),
        (long long)exclusive[0], (long long)exclusive[1], (long long)exclusive[2],
        (long long)exclusive[3], (long long)exclusive[4], (long long)exclusive[5],
        (long long)exclusive[6], (long long)(end - epoch - total),
        calls[0], calls[1], calls[2], calls[3], calls[4], calls[5], calls[6], max_depth);
    for (unsigned i = 0; i < count; ++i) { exclusive[i] = 0; calls[i] = 0; }
    max_depth = 0;
    epoch = esp_timer_get_time(); // Exclude this report's serial-print time.
}
#endif
}
