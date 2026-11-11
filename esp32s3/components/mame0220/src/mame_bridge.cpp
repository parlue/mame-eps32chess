// Bruecke ESP-IDF <-> unveraenderte MAME-0.220-Quellen.
//
// Meilenstein 0: nur ein Selbsttest, der echte MAME-Utility-Funktionen aufruft.
// Damit ist bewiesen, dass Quellen, Include-Layout und Compiler-Schalter auf dem
// xtensa-Ziel zusammenpassen (statt nur "kompiliert ohne Aufruf").
//
// Meilenstein 1 haengt hier den Kern an: running_machine + Treiber mm4.

// Reihenfolge wie in MAMEs emu.h: eminline.h liefert mulu_32x32/divu_64x32_rem,
// die attotime.h in Inline-Funktionen benutzt. Ohne dieses Include bricht der
// Uebersetzer mit "'mulu_32x32' was not declared in this scope" ab.
#include "eminline.h"
#include "attotime.h"

extern "C" int mame_bridge_selfcheck(void)
{
    // mm4 laeuft mit 9.8304 MHz / 2 = 4.9152 MHz. Hier nur Attosekunden-Rechnung.
    attotime twenty_ms = attotime::from_msec(20);      // 50 Hz
    attotime one_second = attotime::from_seconds(1);

    const bool ok_ms = (twenty_ms.as_attoseconds() == 20000000000000000LL);
    const bool ok_s = (one_second.seconds() == 1);
    return (ok_ms && ok_s) ? 1 : 0;
}

extern "C" const char *mame_bridge_note(void)
{
    return "MAME 0.220 original sources, ESP32-S3 build (milestone 0)";
}
