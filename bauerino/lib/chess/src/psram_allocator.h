#pragma once
#include <memory>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <limits>
#include <new>
// Large engine hash storage belongs in PSRAM, independently of malloc policy.
template<class T> struct EngineAllocator {
  using value_type=T;
  EngineAllocator()=default;
  template<class U> EngineAllocator(const EngineAllocator<U>&) {}
  T* allocate(size_t count) {
    if(count>std::numeric_limits<size_t>::max()/sizeof(T)) throw std::bad_alloc();
    void* data=heap_caps_malloc(count*sizeof(T),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!data) throw std::bad_alloc();
    return static_cast<T*>(data);
  }
  void deallocate(T* data,size_t) {heap_caps_free(data);}
};
template<class T,class U> bool operator==(const EngineAllocator<T>&,const EngineAllocator<U>&){return true;}
template<class T,class U> bool operator!=(const EngineAllocator<T>&,const EngineAllocator<U>&){return false;}
#else
template<class T> using EngineAllocator=std::allocator<T>;
#endif
