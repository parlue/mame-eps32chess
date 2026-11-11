// Mini-Test (MAME-unabhaengig): stellen die beiden Compiler-Macros den
// Typzustand her, den MAME 0.220 voraussetzt (int32_t == int, uint32_t ==
// unsigned int, damit auch size_t == uint32_t auf xtensa)?
//
//   (1) std::max(int32_t, int)      -> MAMEs inputdev.cpp:946 / save_item(int)
//   (2) std::min(size_t, uint32_t)  -> MAMEs dipalette.cpp:211
//
// Uebersetzt die Datei, sind beide Typidentitaeten gegeben.

#include <cstddef>
#include <cstdint>
#include <algorithm>

extern "C" int mame_s3_type_probe(void)
{
    int32_t  i = 0;
    uint32_t u = 1u;
    size_t   s = 4u;

    int n = std::max(i, 0);                    // (1) int32_t == int ?
    n += static_cast<int>(std::min(s, static_cast<size_t>(u)));  // (2) size_t == uint32_t ?
    return n;
}
