Status dieser Dateien: NICHT VERDRAHTET (Altlast, Stand Meilenstein 1a)
======================================================================

Die Dateien dipalette.cpp, inputdev.cpp und natkeyboard.cpp in diesem
Verzeichnis sind Kopien der gleichnamigen MAME-0.220-Originale
(ref/mame0220-src/src/emu/), die testweise mit lokalen Typ-Anpassungen
versehen wurden. Sie sind in components/mame0220/CMakeLists.txt NICHT in
SRCS aufgelistet und werden daher nicht uebersetzt.

Warum sie nicht gebraucht werden
--------------------------------
Alle drei Dateien scheiterten an demselben Grundproblem: auf xtensa (newlib)
ist int32_t == long, auf x86/ARM dagegen int32_t == int. MAME 0.220 verlaesst
sich auf die x86/ARM-Identitaet an Stellen wie

  dipalette.cpp:211   std::min(size_t, u32)
  inputdev.cpp:946    std::max(s32, int)
  natkeyboard.cpp:337 Delegate (void*, int) vs. (void*, s32)

Behoben wird das jetzt an der Wurzel und fuer alle Uebersetzungseinheiten der
Komponente gleichzeitig durch zwei Schalter in der CMakeLists.txt:

  -D__INT32_TYPE__=int  -D__UINT32_TYPE__=unsigned int

Nachweis: tools/xtensa_int32_probe.cpp (ohne Schalter exit=1, mit exit=0).
Damit bleiben die MAME-Quellen unveraendert - die urspruengliche Anforderung
dieses Projekts. Die Kopien hier sind nur noch Beleg/Notfallreserve und
koennen geloescht werden, sobald Meilenstein 1a stabil baut (dieses
Verzeichnis steht in keiner Quelldateiliste und beeinflusst den Build nicht).
