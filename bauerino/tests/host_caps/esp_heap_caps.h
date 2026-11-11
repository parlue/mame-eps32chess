#pragma once
// Host execution of the same task-workspace branch; not a PSRAM simulator.
#include <cstdlib>
#include <cstddef>
#include <stdexcept>
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
inline void *heap_caps_malloc(std::size_t size,unsigned caps) {
    if(caps!=(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT))throw std::runtime_error("workspace caps");
    return std::malloc(size);
}
inline void heap_caps_free(void *p){std::free(p);}
