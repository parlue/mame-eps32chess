// Schnelltest ausserhalb des PlatformIO-Builds (mame-s3/tools/xtensa_type_probe.cpp)
//
// Zweck: prueft zwei xtensa-Eigenheiten, die in Lauf #17/#18 auftraten:
//
//   (1) save_item(int)  -> src/emu/save.h:166
//       "static assertion failed: Called save_item on a non-fundamental type!"
//       MAME erlaubt Speichertypen ueber osd::s32/osd::u32; auf x86/ARM ist
//       int32_t == int, auf xtensa ist int32_t == long int.
//
//   (2) std::min(size_t, u32)  -> src/emu/dipalette.cpp:211
//       "no matching function for call to 'min(size_t, long unsigned int)'"
//       Auf xtensa ist size_t == unsigned int, osd::u32 == unsigned long.
//
// Aufruf ueber tools\type_probe.ps1 - drei Varianten in einem Lauf.

#include "emu.h"

#include <algorithm>

extern "C" void mame_s3_type_probe(device_t &device)
{
    // (1) Speichertypen wie in MAMEs diserial.cpp / dirom.cpp
    int  i = 0;
    u32  u = 0;
    device.save_item(i, "probe_int");
    device.save_item(u, "probe_u32");

    // (2) std::min mit gemischten 32-Bit-Typen, wie in dipalette.cpp:211
    size_t depth = size_t(4);
    u32    entry = u32(1);
    int    count = std::min(depth, size_t(8) - entry);
    (void)count;
}
