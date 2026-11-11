# 11 – Plan Meilenstein 1: MAME-0.220-Kern (`src/emu`) für xtensa

Erzeugt: 2026-11-09. Dies ist ein **Plan**, kein Nachweis. Nachweise kommen als
Build-Läufe in `docs/10_build_log.txt`.

## Ausgangslage

Meilenstein 0 ist der Build der abhängigkeitsarmen Utilities plus zlib/utf8proc.
Meilenstein 1 verlangt laut `docs/09`: *„`src/emu`-Kern (Schedule, Memory,
Devices, ROM-Load) kompiliert; Symbole `running_machine`, `device_t` vorhanden“*.

Die Quelldateiliste steht nicht in `docs/`, sondern in MAMEs eigenem Build-Skript
`scripts/src/emu.lua` (`files{...}`, Zeilen 41–260). Sie wird **nicht kopiert**,
sondern in `components/mame0220/CMakeLists.txt` explizit aufgeführt.

## Stufen (jede Stufe endet mit einem `pio run` als Nachweis)

**M1a – Kern ohne Grafik** — **erledigt (Lauf #21)**: alle unten genannten Dateien
übersetzt, 0 Fehler; Nachweis über die Objektdateien (Abschnitt „Nachweis Lauf #21“
in `docs/10_build_log.txt`, Werkzeug `tools/check_core_objects.cmd`). Die Kern-Symbole
stehen noch **nicht** im Abbild, weil der Linker nicht referenzierte Archivglieder
verwirft – das erledigt M1b.
`device.cpp`, `devcb.cpp`, `devcpu.cpp`, `devdelegate.cpp`, `devfind.cpp`,
`diexec.cpp`, `dimemory.cpp`, `distate.cpp`, `dinvram.cpp`, `dirom.cpp`,
`didisasm.cpp`, `diserial.cpp`, `disound.cpp`, `dislot.cpp`, `diimage.cpp`,
`dirtc.cpp`, `dinetwork.cpp`, `dipty.cpp`, `divideo.cpp`, `divtlb.cpp`,
`dipalette.cpp`, `digfx.cpp`, `drawgfx.cpp`, `emualloc.cpp`, `emucore.cpp`,
`emumem.cpp`, `emumem_mud.cpp`, `emumem_hem.cpp`, `emumem_hedp.cpp`,
`emumem_heun.cpp`, `emumem_heu.cpp`, `emumem_hedr0..3.cpp`, `emumem_hedw0..3.cpp`,
`emumem_hep.cpp`, `emumem_het.cpp`, `addrmap.cpp`, `mconfig.cpp`, `machine.cpp`,
`driver.cpp`, `driver.h`, `schedule.cpp`, `save.cpp`, `memarray.cpp`,
`ioport.cpp`, `input.cpp`, `inputdev.cpp`, `output.cpp`, `xtal.cpp`,
`fileio.cpp`, `hashfile.cpp`, `romload.cpp`, `romentry.cpp`, `emuopts.cpp`,
`config.cpp`, `bookkeeping.cpp`, `parameters.cpp`, `validity.cpp`,
`profiler.cpp`, `network.cpp`, `uiinput.cpp`, `natkeyboard.cpp`,
`emu.h`/`emufwd.h` (Header).

**Nicht** dabei: `main.cpp` (definiert MAMEs eigenes `main()` und kollidiert mit
dem Startpfad von ESP-IDF), `src/emu/debug/*` (Debugger-UI), `video/rgbsse.cpp`
und `video/rgbvmx.cpp` (x86-SSE bzw. PPC-VMX), `drivers/empty.cpp` sowie
`http.cpp` (zieht asio und POSIX-`socketpair` nach, Lauf #19).

**M1b – Grafik-/Audio-Gerüst, soweit der Kern es zwingend referenziert**
`video.cpp`, `screen.cpp`, `render.cpp`, `rendfont.cpp`, `rendlay.cpp`,
`rendutil.cpp`, `tilemap.cpp`, `emupal.cpp`, `sound.cpp`, `speaker.cpp`,
`softlist.cpp`, `softlist_dev.cpp`, `video/generic.cpp`. Grund: `running_machine`
hält `video_manager`/`screen_manager` als Mitglieder; die Symbole sind beim Linken
erforderlich, auch wenn `mm4` keine Grafik benutzt.

**M1c – CPU und Geräte**
`src/devices/cpu/m6502/*.cpp` (u. a. `m65c02.cpp`, `m6502.cpp`), Maschinen-
geräte aus `mephisto_mm2.cpp`: `machine/mmboard.cpp`, `machine/74259.cpp`,
`sound/dac.cpp`, `sound/volt_reg.cpp`, `bus/generic/slot.cpp`,
`bus/generic/carts.cpp`.

## Bekanntes Hindernis: erzeugte `.lh`-Layoutdateien

`mephisto_mm2.cpp` bindet `mephisto_mm2.lh` und `mephisto_bup.lh` ein. Diese
Dateien werden von MAMEs Build aus den `.lay`-Dateien **erzeugt** (Verzeichnis
`build/generated/emu/layout/`) und existieren im Quellbaum nicht.

Da die Layouts ausschließlich die Bildschirmdarstellung betreffen und auf dem S3
keine Grafik gezeichnet wird, ist die vorgesehene Lösung:

* eine **Treiber-Anpassung** im Projekt (z. B. `components/mame0220/src/mm4_port.cpp`),
  die den Originaltreiber einbindet bzw. nachbildet und die beiden `#include`
  sowie `set_default_layout(...)` **weglässt**;
* die Originaldatei unter `ref/mame0220-src` bleibt unverändert;
* die Abweichung wird in `docs/09` (Tabelle „Bekannte harte Punkte“) geführt.

## Randbedingungen, die jedes Mal gelten

0. **xtensa-Typen weichen von x86/ARM ab** (belegt mit `tools/type_probe.ps1` und
   `tools/xtensa_int32_probe.cpp`): Auf dieser Werkzeugkette ist `int32_t == long int`
   (nicht `int`), während `size_t == unsigned int` ist. MAME 0.220 setzt die
   x86/ARM-Identität voraus und scheitert deshalb an vier Stellen im Kern
   (`save.h:166` in `diserial`/`dirom`/`divtlb`, `dipalette.cpp:211`,
   `inputdev.cpp:946`, `natkeyboard.cpp:337`). Lösung (Läufe #19–#21): die Komponente
   setzt zwei Schalter, die den Typzustand von x86/ARM **innerhalb ihrer Übersetzung**
   herstellen:

       -D__INT32_TYPE__=int  -D__UINT32_TYPE__=unsigned int

   Damit bleiben die MAME-Quellen unverändert. Der frühere Ansatz, den Typ per
   erzwungenem Include (`include/mame_s3_xtensa_types.h` mit `ALLOW_SAVE_TYPE(int)`)
   zu erweitern, ist **verworfen** und wird nicht mehr eingebunden: er hätte nur
   `save.h` geheilt, nicht die `std::min`/`std::max`/Delegate-Fälle. Wirkungsnachweis:
   `tools/xtensa_int32_probe.cpp` meldet ohne Schalter `exit=1`, mit Schaltern `exit=0`;
   Lauf #21 übersetzt daraufhin den ganzen Kern mit **0 Fehlern**. Nebenerscheinung:
   95 Warnzeilen `"__INT32_TYPE__" redefined` (harmlos, nur Warnung, kein `-Werror`).

1. **Vor jeder Änderung an einer Komponenten-`CMakeLists.txt`** die Datei
   `.pio/build/esp32s3-skeleton/CMakeCache.txt` löschen – sonst läuft CMake nicht
   erneut und die Änderung wirkt nicht (belegt in Lauf #9/#10, siehe docs/09).
2. **C++-nur-Schalter** (`-frtti`, `-std=gnu++17`) nur in der Komponente setzen,
   niemals global (Lauf #11: `wpa_supplicant` bricht mit `-Werror` ab).
3. **Keine globalen `-I`-Pfade** auf MAME-Quellen – sie verdecken IDF-Header
   (Lauf #13: `crypto_mbedtls.c` bekam MAMEs `sha1.h` → `osdcore.h` → `cstdio`).
4. Jeder Stufenabschluss wird in `docs/10_build_log.txt` mit Laufnummer,
   Ergebnis, Firmwaregröße und freiem Heap festgehalten.
