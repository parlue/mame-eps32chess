#pragma once

// One MAME worker executes the selected machine. Match the exact interface
// identity so other CPUs/interfaces retain their original virtual operations.
// This context borrows everything; the owning interface clears it on teardown.
namespace {
struct s3_direct_memory_context {
    const void *owner = nullptr;
    s3_fast_memory *pages = nullptr;
    memory_access_cache<0, 0, ENDIANNESS_LITTLE> *data = nullptr;
    memory_access_cache<0, 0, ENDIANNESS_LITTLE> *args = nullptr;
    memory_access_cache<0, 0, ENDIANNESS_LITTLE> *opcodes = nullptr;
};
s3_direct_memory_context s3_direct_memory;

// Keep these shared: expanding the cache fallback in hundreds of instruction
// bodies increases the interpreter's instruction-cache footprint considerably.
#ifdef S3_DIRECT_MEMORY_FORCE_INLINE
#define S3_DIRECT_MEMORY_ATTRIBUTE __attribute__((always_inline)) inline
#else
#define S3_DIRECT_MEMORY_ATTRIBUTE __attribute__((noinline))
#endif
template<class Interface>
S3_DIRECT_MEMORY_ATTRIBUTE u8 s3_direct_read(Interface *intf, u16 address) {
    if (s3_direct_memory.owner == intf)
        return s3_direct_memory.pages->read(address, *s3_direct_memory.data);
    return intf->read(address);
}
template<class Interface>
S3_DIRECT_MEMORY_ATTRIBUTE u8 s3_direct_arg(Interface *intf, u16 address) {
    if (s3_direct_memory.owner == intf)
        return s3_direct_memory.pages->read(address, *s3_direct_memory.args);
    return intf->read_arg(address);
}
template<class Interface>
S3_DIRECT_MEMORY_ATTRIBUTE u8 s3_direct_opcode(Interface *intf, u16 address) {
    if (s3_direct_memory.owner == intf)
        return s3_direct_memory.pages->opcode(address, *s3_direct_memory.opcodes);
    return intf->read_sync(address);
}
template<class Interface>
S3_DIRECT_MEMORY_ATTRIBUTE void s3_direct_write(Interface *intf, u16 address, u8 value) {
    if (s3_direct_memory.owner == intf)
        s3_direct_memory.pages->write(address, value, *s3_direct_memory.data);
    else
        intf->write(address, value);
}
#undef S3_DIRECT_MEMORY_ATTRIBUTE
}
