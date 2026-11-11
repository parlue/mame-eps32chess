// ---------------------------------------------------------------------------
// ABGELEGT / NICHT MEHR VERWENDET (Stand: Lauf #21, 2026-11-09).
//
// Diese Datei wird von components/mame0220/CMakeLists.txt NICHT mehr per
// "-include" erzwungen. Der Ansatz ist verworfen, weil er nur save.h geheilt
// haette und nicht die uebrigen Typfaelle:
//   dipalette.cpp:211  std::min(size_t, u32)      -> kein std::min-Kandidat
//   inputdev.cpp:946   std::max(s32, int)         -> kein std::max-Kandidat
//   natkeyboard.cpp:337 Delegate (void*, int)     -> passt nicht zu (void*, s32)
//
// Stattdessen stellt die Komponente den Typzustand von x86/ARM mit zwei
// Schaltern her (Wirkungsnachweis tools/xtensa_int32_probe.cpp, Lauf #21):
//     -D__INT32_TYPE__=int   -D__UINT32_TYPE__=unsigned int
// Damit sind alle vier Faelle an der Wurzel behoben, ohne eine Quellzeile zu
// aendern. Die Datei bleibt nur als Nachweis/Notfallreserve liegen; sie steht in
// keiner Quelldateiliste und beeinflusst den Build nicht.
//
// Zur Information - der urspruengliche Befund (Lauf #17):
// ---------------------------------------------------------------------------
// Plattform-Anpassung fuer xtensa (ESP32-S3), KEINE Aenderung an MAME-Quellen.
//
// Problem (belegt im Buildlauf #17, docs/10_build_log.txt):
//   MAME erlaubt Speichertypen in src/emu/save.h (Zeilen 321-336) ueber
//       ALLOW_SAVE_TYPE_AND_ARRAY(osd::s32)   // = int32_t
//       ALLOW_SAVE_TYPE_AND_ARRAY(osd::u32)   // = uint32_t
//   Auf x86/ARM ist int32_t == int, deshalb deckt das automatisch `int` mit ab.
//   Auf xtensa (newlib) ist int32_t == long int - `int` bleibt damit
//   unabgedeckt und die Voreinstellung type_checker<T>::is_atom = false greift:
//       save.h:166: error: static assertion failed:
//                   Called save_item on a non-fundamental type!
//       note: 'save_manager::type_checker<int>::is_atom' evaluates to false
//                   required from diserial.cpp:72
//
// Nur auf dieser Werkzeugkette noetig: waere int32_t == int, waeren diese beiden
// Zeilen eine Doppeldefinition und wuerden nicht uebersetzen.

#pragma once

#include "emu.h"

ALLOW_SAVE_TYPE(int);
ALLOW_SAVE_TYPE(unsigned int);
