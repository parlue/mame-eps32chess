# 13 – Lauf #24: Befunde und Änderungen am Linkversuch (Übergabe)

Erzeugt: 2026-11-10, ~13:55. Grundlage: der am 2026-11-10 um 12:19
fehlgeschlagene Abschlusslink (671 × `undefined reference`, 114 verschiedene
Symbole; 18 × `dangerous relocation: l32r`) und die danach abgebrochenen Läufe
13:04, 13:28, 13:36, 13:47, 13:51.

**Stand: kein erfolgreicher Link. Kein `firmware.bin` erzeugt, nichts geflasht.**
`esp32s3/.pio/build/esp32s3-skeleton/firmware.bin` ist weiterhin das Abbild vom
2026-11-09 13:09 (218.496 B). Der Lauf 13:51 wurde um 13:53:45 abgebrochen
(alle Compilerprozesse gestoppt), 124 `.obj` liegen im Buildordner.

## 1. Umgebung (Voraussetzung, unabhängig vom Code)

* **Die PlatformIO-`penv` war zerstört.** Am 2026-11-10 12:37 wurden in
  `%USERPROFILE%\.platformio\penv\Lib\site-packages` Pakete verändert
  (`charset_normalizer`, `websockets` neu) und dabei **`urllib3` und `chardet`
  entfernt**. Folge: jeder `platformio`-Aufruf endete nach 1 s mit
  `ModuleNotFoundError: No module named 'urllib3'` (Beleg: `docs/raw_err.txt`
  des Laufs 13:02). **Vor der Reparatur war kein Build möglich.**
  Behoben durch `pip install "urllib3<3"` → `urllib3 2.8.0`; Kontrolle:
  `platformio --version` → `PlatformIO Core, version 6.2.0`.
* Als Nebenwirkung fehlte auch `tool-esptoolpy`; PlatformIO hat es beim Lauf
  13:04 selbst nachinstalliert (`docs/raw_out.txt`).
* Übrig bleibt eine harmlose `RequestsDependencyWarning` (unvollständiger
  `charset_normalizer`-Stand aus 12:37) – sie steht in jeder `raw_err.txt` am
  Anfang und ist kein Buildfehler.

## 2. Ursachen des Fehllinks (belegt, nicht vermutet)

Alle Zuordnungen stammen aus MAMEs eigenem Bauplan im Baum
(`ref/mame0220-src/scripts/src/…`) und aus `docs/raw_err.txt`.

| Befund | Beleg |
|---|---|
| `driver_list::find(char const*)` fehlte, weil `src/emu/drivenum.cpp` nicht gebaut wurde | Definition steht in `src/emu/drivenum.cpp:38`, nicht in der erzeugten `drivlist.cpp`; `s3_startup.cpp` ruft sie in `mame_s3_target()`/`mame_s3_run()` |
| Die `utils`-Bibliothek war unvollständig | `scripts/src/lib.lua:28-118` (MAMEs eigene Liste) vs. `CMakeLists.txt` |
| Expat, pugixml, 7-Zip/LZMA, libFLAC fehlten vollständig | `scripts/src/3rdparty.lua:17-75` (expat), `:711-791` (7z), `:618-699` (flac); pugixml als extlib: `scripts/extlib.lua:21` |
| `chd.cpp` verlangt `chd_codec_list` (also `chdcodec.cpp` → `flac.cpp` → libFLAC) | `src/lib/util/chd.cpp:2292/2456` |
| Literal-Pools zu weit entfernt | Linkermeldung „try using text-section-literals“ |
| **Laufzeitabsturz, zusätzlich gefunden:** `running_machine::run()` ruft bedingungslos `m_manager.http()->clear()`; ohne `start_http_server()` bleibt `m_manager.http()` nullptr | `src/emu/machine.cpp:306` und `:383`; Anlage nur in `src/emu/main.cpp:21-24`; `main.h:98` |

## 3. Änderungen (`esp32s3/components/mame0220/`)

`CMakeLists.txt`:

1. `MAME_UTIL_SRC` um MAMEs `utils`-Liste ergänzt: `avhuff, aviio, bitmap,
   cdrom, chd, chdcd, chdcodec, corefile, flac, harddisk, huffman, jedparse,
   nanosvg, palette, plaparse, png, pool, un7z, vbiparse, wavwrite, xmlfile,
   zippath` (+ expat ×3, pugixml, 7z/LZMA ×30, libFLAC ×15, jeweils mit den
   Original-Schaltern aus `3rdparty.lua`).
2. `MAME_EMU_SRC` um `main.cpp`, `http.cpp`, `drivenum.cpp` ergänzt.
3. Include-Pfade: `3rdparty/lzma/C`, `3rdparty/libflac/include`,
   `3rdparty/libflac/src`, `3rdparty/libflac/src/libFLAC/include`.
4. Schalter: `-mtext-section-literals`, `-mlongcalls`;
   `-include <src/xtensa_posix_shim.h>`; `CRLF=2` (genie.lua:629-638).
5. Dateibezogen: expat/lzma/libFLAC/pugixml mit
   `-Wno-error=maybe-uninitialized`; pugixml zusätzlich mit
   `LLONG_MIN/LLONG_MAX/ULLONG_MAX`; libFLAC **ohne** `HAVE_CONFIG_H`, dafür
   `VERSION="1.2.1"`.

`src/s3_startup.cpp`:

* Ersatzkopie des `machine_manager`-Konstruktors entfernt (kommt jetzt mit
  MAMEs `main.cpp`), dafür `manager.start_http_server()` vor `machine.run()`
  (verhindert den Nullzeiger oben).
* 16 `emulator_info`-Funktionen ersetzt (Original:
  `src/frontend/mame/mame.cpp:375-432`).

Neu: `src/xtensa_posix_shim.h` – **nur** Deklaration von `socketpair` plus
`<limits.h>`/`LLONG_*`-Rückfall. Grund: asio übersetzt
`socket_ops::socketpair` (`asio/detail/impl/socket_ops.ipp:623`) inline, lwIP
kennt `socketpair` nicht, die Funktion wird nie aufgerufen – es bleibt bei der
Deklaration. Keine MAME-Quelle geändert.

## 4. Beobachtete und behobene Übersetzungsfehler (Reihenfolge der Läufe)

1. `corefile.cpp:35: #error CRLF undefined` → `CRLF=2`.
2. `expat/xmltok.h:256: 'buf' may be used uninitialized [-Werror=…]` →
   `-Wno-error=maybe-uninitialized`. Merke: ESP-IDF setzt `-Werror=all`
   (belegt in `build.ninja`); ein **spezifisches** `-Werror=` schlägt das
   allgemeine `-Wno-error`, deshalb half der bisherige Schalter hier nicht.
3. `pugixml.cpp:4564: 'LLONG_MIN/LLONG_MAX/ULLONG_MAX' undeclared` → `-D`-Werte
   (der Shim definiert sie zusätzlich, damit die Stelle nicht von der
   Include-Reihenfolge abhängt).
4. `libFLAC bitwriter.c/cpu.c: fatal error: cstdio: No such file or directory`:
   libFLACs `#include <config.h>` fand **MAMEs** `src/emu/config.h` (beide
   Include-Wurzeln liegen in einer Komponente; die `-I`-Reihenfolge lässt sich
   nicht pro Datei trennen) und MAMEs config.h zieht `xmlfile.h → osdcore.h →
   osdcomm.h → <cstdio>`, was in einer C-Datei fatal endet. → `HAVE_CONFIG_H`
   für die libFLAC-Dateien entfernt.
5. dadurch `format.c:60/66: 'VERSION' undeclared` → `VERSION="1.2.1"`
   (wortgleicher Inhalt von libFLACs `config.h`, die nur diese eine Zeile hat).
   **Gesetzt, aber nicht mehr verifiziert** – der Lauf wurde vorher abgebrochen.

Zählhinweis: in `raw_err.txt` nach `error:` suchen, nicht nach `: error:` –
sonst werden `fatal error:`-Zeilen übersehen.

## 5. Offene Punkte (in dieser Reihenfolge)

1. Lauf neu starten und Punkt 5 aus §4 bestätigen:
   `tools\run_pio_build.cmd` (abgekoppelt) → `docs\raw_out.txt` / `raw_err.txt`;
   Fortschritt `docs\_watchloop.txt`; Abschluss über die Zeile `exit=`.
   **Ausgabe nicht in den VS-Terminal leiten** – die 346-KB-Flut des Laufs 12:19
   hat die Oberfläche zum Absturz gebracht.
2. **Noch nie übersetzt:** `src/emu/http.cpp` (asio + Shim) sowie
   `chd/chdcd/chdcodec/flac` (C++).
3. **Der eigentliche Link steht aus:** die 114 Symbolgruppen und 18
   l1 r-Relocations müssen verschwinden; erst dann ist §2 belegt.
4. Danach: ROM-Dateisystem `/roms` mounten, Treiberauswahl statt fest `mm4`,
   ioport-/output-Zugriff – siehe
   `integration/chessengine/MESSCHESS-ANFORDERUNGEN.md`.

## 6. Bedien- und Randhinweise

* Backup-Stand: `mame-s3/backups/link24-20261110/` (CMakeLists.txt,
  s3_startup.cpp) – **Stand 13:03, also schon mit den ersten Änderungen**,
  kein Urzustand. Meine Eingriffe sind im Code mit `Lauf #24` markiert.
* Im Ordner `ChessL1nkwireless` existiert **kein Git-Repo** (`git status` →
  „not a git repository"); es gibt keine Versionssicherung.
* Der Bau bricht bisher nach 2–3 Minuten am ersten Fehler ab; ein
  durchlaufender Kompilat braucht ~20–25 Min, das Linken der 471-MB-Bibliothek
  danach noch einmal spürbar Zeit. Bei zäher Maschine `-j 4` statt `-j 6`
  (Speicherdruck; siehe Kommentar in `tools/run_pio_build.cmd`).
