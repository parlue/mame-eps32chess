#pragma once
// MAME assumes int32_t is int; both Xtensa types are 32-bit ABI-compatible.
// Apply before libc headers, only in MAME translation units.
#undef __INT32_TYPE__
#undef __UINT32_TYPE__
#define __INT32_TYPE__ int
#define __UINT32_TYPE__ unsigned int
