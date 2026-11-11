# MAME-S3 Übernahme / ROM-Bring-up

Stand: 10.11.2026. Zielgerät: Waveshare ESP32-S3-Touch-AMOLED-2.16,
480 × 480, 16 MiB Flash, 8 MiB Octal-PSRAM. USB: 303a:1001,
Seriennummer 44:BD:8D:60:CB:44 (aktuell COM8).

## Sicherung

Vor jedem MAME-Flash wurde das komplette vorhandene Flash gelesen:
`backups/takeover-20261110/waveshare-before-mame-16MB.bin`.
16.777.216 Bytes; SHA256:
`4264EB20D86CFD878D9135853A6B27C1EC817379FA63FBFC15245250C38A8E1E`.
Die Datei enthält die bisherige Bauerino-Demo und deren Daten.
Das neue Standalone-Image enthält vorerst keine Displayoberfläche.

## Bereits belegt

- Portable UCI-/Lua-Brücke mit den echten MessChess-Profilen MM IV und MM V:
  Hosttests für Protokoll, Brettsteuerung und Sonderzüge erfolgreich.
- Prüfung gegen echte ROMs unter unverändertem PC-MAME 0.220:
  MM IV antwortet auf e2e4 mit e7e5, MM V mit c7c6.
  Protokolle: `integration/chessengine/build-host/rom-tests/`.
- Runtime + 28 Lua-C-Dateien für Xtensa übersetzt; echte MAME-Adapter-API
  gegen die Originalheader geprüft. Noch kein Beleg für einen ESP-ROM-Start.
- Privates FAT/Wear-Levelling-Image für die vorhandene 4-MiB-ROM-Partition
  erzeugt; Mountcode formatiert bei Fehlern ausdrücklich nicht.

## Lokale Plattformänderungen

Die Originaldateien unter `ref/mame0220-src` werden nicht bearbeitet.

- Minimale echte `ui_manager`-Instanz: Core-ioport/video dereferenzieren sie
  auch ohne Desktop-Menü.
- Unsichtbares Render-Target: Core-Konfigurationsspeicherung braucht es.
- ESP-Mikrosekundentimer und FreeRTOS-Sleep bleiben erhalten; Wrapper trennt
  sie von den gleichnamigen Definitionen in Original-`osdcore.cpp`.
- MAMEs interner Desktop-HTTP-Server ist ausdrücklich deaktiviert. Dessen
  Originalkonstruktor legt selbst bei `active=false` einen ASIO-Kontext an.
  Der lokale inaktive Vertrag vermeidet fehlende POSIX-Desktopfunktionen.
  Aktivieren wird mit Fehler abgewiesen; Bauerino hat seinen eigenen Server.
- Original-JPEG-Bibliothek und originale Netzwerkgeräteverwaltung ergänzt;
  letztere registriert keine physischen Adapter.
- Boardprofil: Octal-PSRAM 80 MHz, CPU 240 MHz, primäre USB-Konsole,
  FAT-Langnamen für `mephisto5.rom`.
- Alle fünf Sekunden tatsächliche emulierte Zeit / Echtzeit sowie freier
  interner Heap und PSRAM. Leerlaufwerte belegen keine Suchgeschwindigkeit.

## Offene Nachweise

Vollständiger Link, Partitionsgrößenprüfung, Boot mit erkanntem PSRAM,
ROM-Maschinenstart und reale UCI-Antwort auf dem ESP. Danach erst Messungen
während der Suche und Integration mit Display, Funkbrett und WLAN.

Die lokale USB-UCI-Transportdatei ist vorbereitet, aber noch nicht in den
aktiven Kern eingebunden. Sie greift ausschließlich im Emulationstask auf
MAME/Lua zu und begrenzt die Eingabearbeit pro Frame.

## Erster vollständiger Link

`docs/takeover-build-03.log`: erfolgreich, 1127 Sekunden. PlatformIO meldet
8.970.620 Bytes Flash und 58.148 Bytes statisches RAM. Die erzeugte
`firmware.bin` hat 8.970.992 Bytes. Das sind keine Heap-Messwerte.

Das Standalone-Layout wurde deshalb auf 11 MiB App ab 0x10000 und 4 MiB
FAT/WL ab 0xb10000 erweitert. Es bleibt innerhalb der 16 MiB Flash.
Das Ausdünnen des Kerns ist auf ausdrücklichen Nutzerwunsch vertagt.

Die alte `partitions.bin` des PIO-Buildordners blieb trotz geändertem CSV
liegen; die Prüfung in `tools/flash_s3_bringup.py` hat das vor jedem Schreiben
abgewiesen. Bootloader und Partitionstabelle werden daher explizit mit dem
generierten IDF-Ninja-Target `bootloader` gebaut. Dessen abschließende
Imagekonvertierung benötigt das bereits vorhandene Bauerino-Python mit
esptool; die reine IDF-Pythonumgebung enthält das Modul nicht. Der Flasher
benutzt `bootloader/bootloader.bin` und erzeugt seine Partitionstabelle
`bringup-partitions.bin` direkt aus dem aktuellen CSV mit IDFs Generator.
Er prüft Partitionsgrenzen, ROM-Imagehash, Backuphash und USB-Geräteidentität.
Erst `--write` schreibt.

## Erster Gerätetest

`takeover-flash-02.log`: Bootloader, Tabelle, App und ROM-Dateisystem
erfolgreich geschrieben und geprüft. `takeover-boot-01.log` belegt:
240 MHz CPU, 16 MiB Flash, 8 MiB Octal-PSRAM mit 80 MHz und bestandenem
Speichertest; FAT/WL gemountet, MM-IV-ROM vorhanden. Der Kern erreicht die
Erstellung der CPU-ROM-Region und stürzt anschließend in `image_manager`
ab. Der Backtrace zeigt eine fehlende `image_option`.

Ursache im lokalen Startpfad: `options.set_system_name(driver.name)` fehlte.
Das Originalfrontend ruft es auf und erzeugt damit Sensorboard-/Imageoptionen.
Der Aufruf ist ergänzt. Gleichzeitig ist die vorbereitete native UCI-Konsole
nun im Build registriert. Sie wird erst bei `machine_phase::RUNNING`
geöffnet und vor der Zerstörung der Maschine geschlossen.

Build 04 brach wegen einer zu früh ausgeführten CMake-Source-Property ab.
Nach der Korrektur verwendete Build 05 noch den alten generierten Dateisatz;
dieser eigene Linklauf wurde beendet. Build 06 erzwingt die Neuerzeugung;
seine Compile-Datenbank enthält Runtime, Konsole und die 28 Lua-C-Dateien.
Das Gerät wartet währenddessen im USB-Bootloader, statt ständig neu zu starten.

Zusatzprüfung: `tools/xtensa_delegate_probe.cpp` wurde mit dem tatsächlichen
Xtensa-GCC in Assembler übersetzt (`docs/xtensa-delegate-probe.s`). Virtueller
Methodenzeiger = `{1,0}`, direkter Zeiger = `{Funktion,0}`. Das stimmt mit
MAMEs ausgewähltem internen Delegate-Decoder überein; eine spekulative
Umstellung auf andere Callback-Implementierungen ist nicht erforderlich.

## Zweiter Gerätetest: Systemauswahl korrigiert

Build 06 erfolgreich (537 Sekunden): 9.104.488 Bytes Flash,
58.188 Bytes statisches RAM. `takeover-flash-03.log` bestätigt das Schreiben
und die Hashprüfung. `takeover-uci-01.log` zeigt nun CPU-, Timer-, Latch-,
Sensorboard-, PWM-, Spannungsregler- und Slotstart. Beim DAC-Start wird
`std::bad_alloc` abgefangen; MAME beendet sich mit Ergebnis 3. Es gab noch
kein `[UCI READY]` und keinen ESP-ROM-Antwortzug.

Build 07 ergänzt einen Heap-Fehlercallback mit angeforderter Bytezahl,
Capabilities, freiem/größtem internem und externem Block sowie Backtrace.
Zusätzlich werden unmittelbar vor dem Gerätestart Audio-Abtastrate,
Updateperiode und freier Speicher protokolliert. Das dient der konkreten
Fehlerbehebung; der allgemeine Funktionsabbau bleibt vertagt.

## Konkreter Speicherbefund

Build 07 und `takeover-uci-02.log` bestätigen eine normale Audio-Updateperiode
von 20 ms. Vor Gerätestart: 50.127 Bytes interner Heap, 46.360 Bytes PSRAM
(größter externer Block 37.888). Beim zweiten DAC-Start fehlen 30.728 Bytes
für einen Eingangs-Resamplepuffer; zu diesem Zeitpunkt sind extern nur noch
2.156 Bytes frei. Der interne 32-KiB-Reservepool ist für allgemeines malloc
geschützt. Kein Indiz für einen durch fehlerhafte Zeitrechnung riesigen Puffer.

Ein belegter unnötiger Anfangsbedarf: `sound_manager` legt seine Vektoren
zunächst für die voreingestellten 48.000 Samples an (`s16` + zweimal `s32`).
Erst im Konstruktorrumpf setzt `no_sound` die effektive Maschinenrate auf
11.025 Hz. Build 08 setzt daher den originalen `samplerate`-Optionswert schon
vor dem Maschinenstart auf 11.025. Das spart 369.750 Bytes Vektornutzdaten.
CPU-Takt, ROM und Geräte bleiben erhalten. Gerätetest steht noch aus.

Build 08 erfolgreich (415 Sekunden), 9.104.960 Bytes Flash. Flash 05 und
`takeover-uci-03.log`: vor Gerätestart jetzt 416.108 Bytes PSRAM statt 46.360,
größter Block 401.408. Der DAC und der letzte Root-Gerätestart funktionieren.
Danach fängt MAME `std::logic_error` mit
`basic_string: construction from null is not valid` ab. Noch kein UCI READY.

Build 09 ergänzt eine gezielte Diagnose via `--wrap=__cxa_throw`. Nur für
`logic_error` wird der Throw-Backtrace gedruckt; danach ruft der Wrapper den
originalen C++-Exceptionmechanismus auf. Normale MAME-Exceptions für
Geräteabhängigkeiten bleiben unverändert. Der Linkschalter ist vorerst in
`platformio.ini` eingetragen und löste eine größere SDK-Neuübersetzung aus.

Build 09 erfolgreich (2455 Sekunden). Flash 06 vollständig geschrieben und
hashgeprüft. `takeover-uci-04.log`: Throw-Backtrace zeigt
`configuration_manager::load_settings()`, config.cpp:85, beim cfg_directory.
Objektdatei enthält die unterschiedlichen Literale `cfg_directory` (0x58) und
`default.cfg` (0x66). Im gelinkten Code laden beide Stellen dagegen denselben
Zeiger auf `default.cfg`. Daher wird eine nicht existierende Option abgefragt
und nullptr an std::string übergeben. Build 10 prüft als reine Linkänderung
`--no-relax` (Post-Skript, keine Compilerflags geändert). Ursache und Erfolg
müssen noch am neuen Binary und Gerät bestätigt werden.

Isolierte Reproduktion gelungen: `tools/xtensa_literal_probe.cpp` und
`tools/xtensa_literal_anchor.cpp`, GCC/ld 13.2.0+20240530, -Os,
-mtext-section-literals, -mlongcalls. Ein Format-Endzeiger inklusive Nullzeichen
ist im ursprünglichen Stringpool numerisch gleich dem Beginn des nächsten
Strings (`ctrlr`). Nach String-Merging aus mehreren Objektdateien sind die
Adressen verschieden. Der relaxierende Xtensa-Linker verwendet trotzdem den
zusammengeführten Literalplatz für beide und liefert für `ctrlr` stattdessen
`default.cfg`. `docs/xtensa-literal-probe.txt` zeigt den falschen Zugriff;
`docs/xtensa-literal-no-relax.txt` zeigt mit `--no-relax` wieder getrennte,
richtige Zeiger. Damit ist die Linkkorrektur unabhängig von MAME bestätigt.
Der Gerätetest nach dem Firmware-Neubau bleibt erforderlich.

Build 10: Übersetzung fertig, Link nach 787 Sekunden fehlgeschlagen. Globales
`--no-relax` ist für IDF 5.3 ungeeignet: acht Ausnahme-/Interruptvektoren benötigen
die Literalverschiebung des Linkers (literal placed after use). Nicht geflasht.
Gezielter, ebenfalls im Minimalbeispiel bestätigter Ersatz: nur die
MAME-Komponente mit `-fno-merge-constants` kompilieren, SDK und Linker normal.
`docs/xtensa-literal-no-merge.txt` zeigt korrekte Zeiger. Die Änderung steht in
der Komponenten-CMakeLists; Build 11 läuft ohne diagnostisches Post-Skript.
`no_relax_link.py` bleibt ausschließlich als dokumentierter Diagnoseversuch.

## Erster echter ESP-ROM-Zug bestätigt

Build 11 erfolgreich gelinkt (9.138.232 Bytes Flash). Der anschließende
PlatformIO-Verbose-Ausgabefehler (`_Null` + str in SCons) wurde durch direkten
elf2image-Aufruf mit esptool 5.1 umgangen. Vollständiges Binary verifiziert:
cfg_directory-Zugriff wieder korrekt (Details docs/15). Flash 07 vollständig
geschrieben und hashgeprüft.

`takeover-uci-05.log` besteht den tatsächlichen Gerätetest:

* `[UCI READY] mm4; Lua heap=112128; USB console`
* Identifikation `Mephisto MM IV (v7.10)`, Optionen Speed und Level
* uci/uciok, zweimal isready/readyok, Level le 1, Speed 0
* `position startpos moves e2e4`, `go` -> **`bestmove e7e5`**
* `quit` -> `Maschine beendet, Ergebnis 0`
* PASS: actual ESP ROM + UCI, bestmove e7e5, clean machine exit

Erste Rate auch nach go nur rund 17,3 bis 17,8 Prozent. Das ist ein echter
Funktionsnachweis, noch keine Originalgeschwindigkeit. Dedizierter Suchtest
steht aus; eine Buchantwort ist kein belastbarer Suchbenchmark. Nach dem
ersten ROM-Zug noch ca. 62 KiB PSRAM frei. UI/BLE/WLAN laufen in diesem
Standalone-Test nicht und sind nicht in der gemessenen Speichermenge enthalten.
Der zugesagte Screenshot wird aus dieser belegten Zugfolge mit den originalen
Retro-Zeichenfunktionen erzeugt, ausdrücklich als Desktop-Render gekennzeichnet.

Screenshots visuell geprüft und mit Testlog/evidence.json im Produktprojekt
`espmame/docs/mame-first-move-20261110` abgelegt. Erste funktionierende Firmware
und ELF gesichert als `backups/takeover-20261110/mm4-first-move-firmware.*`.
Firmware-SHA256: d15e3120559eabdfab424b41f8a1da4a310939d0afe3a827ec4784fdd2a6172d.

Benchmark 01 (`takeover-benchmark-01.log`): le 9, Speed 0, erster Zug h2h4,
60 Sekunden ab go, danach quit und sauberer Ergebnis-0-Ausstieg. Kein Antwortzug
in diesem begrenzten Lauf. Durchschnitt laut MAME 15,4 Prozent. Mehrfach
physische PSRAM-Allokationsfehler bei 128 Bytes, durch Lua-Notfall-GC aufgefangen.
Backtrace: Runtime::allocate -> luaM_realloc_ -> Lua-Tabellenvergrößerung im
Coroutine-Interpreter. Freier Speicher pendelt zwischen fast null und ca. 117 KiB.
Damit ist dieser Lauf zusätzlich durch Speicherbereinigung/Diagnoseausgabe belastet.

Gezielte Runtime-Korrektur (keine MAME-Funktionsentfernung): nach Laden der
Factories vollständige Sammlung, GC-Pause 110 Prozent statt 200 und ein kleiner
inkrementeller GC-Schritt pro Poll. Vorhandene Hosttests bestanden, VM nach
Handshake nun 105.866 Bytes am Desktop statt rund 207 KiB. Build 12 erfolgreich
in 120,66 Sekunden, Flashbedarf 9.138.268 Bytes. Gerätetest nach Flash 08 folgt.

Flash 08 und Benchmark 02 bestanden: identischer h2h4/le-9/Speed-0-Lauf,
60,19 Sekunden ab go, danach sauberer quit. Keine `[ALLOC FAIL]` mehr.
Anfangs freies PSRAM ca. 134 KiB statt 96 KiB; während des längeren Laufs
ca. 69 bis 134 KiB. MAME-Durchschnitt 15,22 Prozent, also durch GC-Korrektur
keine erreichte Originalgeschwindigkeit. Stabile GC-Firmware und ELF separat
gesichert als `mm4-gc-stable-firmware.*`.

Build 13 ergänzt MM V in der sortierten Treibertabelle und den öffentlichen
C-Einstieg `mame_s3_run_driver(name)`; der bisherige MM-IV-Einstieg bleibt.
Nach quit werden VM/UI/Maschine in definierter Reihenfolge zerstört und der
Manager-Maschinenzeiger gelöscht. Die Standalone-Firmware akzeptiert vor dem
Start `load mm4` oder `load mm5` per USB, startet anfangs ohne Eingabe nach
drei Sekunden MM IV und wartet nach quit auf die nächste Load-Anforderung.
Testwerkzeug unterstützt `--driver mm4|mm5`. Tatsächliche Auswahl und
Wiederverwendung desselben gestarteten ESP-Prozesses müssen noch getestet werden.

Build 13 erfolgreich, Flash 09 und Load-Zyklus 01: MM V meldet sich korrekt,
antwortet auf e2e4 mit **c7c5**, und beendet sich mit Ergebnis 0. Anschließender
MM-IV-Start scheitert: nach MM-V-Ende nur noch 1.628.188 Bytes PSRAM frei
(vorher 8.385.916), neue Allokation 3.145.744 Bytes nicht möglich. Backtrace
zeigt `address_space_specific<0,0,0>`-Konstruktor in emumem.cpp:503. Original
0.220 besitzt dort keinen abgeleiteten Destruktor zur Freigabe der beiden
rohen Read/Write-Rootzeiger. Noch keine Änderung/Overlay am Speicherkerne-Code.
ROM-Wechsel ohne Neustart ist damit **nicht** bestanden.

## Priorität: echte Anzeige auf dem Waveshare

Nutzer hat klargestellt: Screenshot soll **im LCD/auf dem Gerät in der
Retrooberfläche** entstehen, nicht nur als Desktop-Render. Bestätigt mit
„ja bitte“. Deshalb ist die nächste aktive Aufgabe die direkte Anbindung des
CO5300 an den laufenden MAME-Testkern. Die bestehende Arduino-Displayfirmware
ist weiterhin nicht mit dem IDF-5.3-Kern zusammengeführt.

Implementiert: `src/bauerino_retro_lcd.c/.h`, direkter IDF-QSPI-Treiber mit
Originalpins CS12/CLK38/D0..3=4,5,6,7/RESET39, MADCTL 0xa0 wie bisher.
Originales drawRetro()-Hintergrundbild als 460.800-Byte-Flash-Asset; nur eine
960-Byte-DMA-Pixelzeile im RAM. Obere vier Stellen zeigen MAME-Outputs digit0..3,
untere vier Stellen den echten letzten bestmove. Noch keine Touch-Anbindung.
`screenshot` im nativen USB-Transport zeichnet das vollständige Bild auf das
Panel und liefert dieselben RGB565BE-Pixel über USB. `test_s3.py --screenshot`
speichert diesen tatsächlichen Geräte-Pixelstrom nach einem ROM-Zug als PNG.
Keine Kameraufnahme und kein Panel-Readback; derselbe Pixelstrom geht an QSPI.

Build 14 erkannte CMake/Assets korrekt, scheiterte nur an einer Formatwarnung
in der neuen C-Datei (-Werror=misleading-indentation). Korrigiert; Build 15
läuft inkrementell. Nächste Schritte: Build fertigstellen, Flash, MM-IV-Zug mit
`--screenshot` prüfen, tatsächliche Anzeige vom Nutzer bestätigen lassen.
Hotload-Speicherproblem danach weiterführen; allgemeinen Funktionsabbau hat
der Nutzer ausdrücklich vertagt.

### Letzte Nutzersteuerung: gerenderte Retro-LCD-Ansicht genügt vorerst

Auf Nachfrage zu „vielleicht reicht das ja“ hat der Nutzer ausdrücklich
„Das gerenderte Bild genügt vorerst“ gewählt. Danach konkretisiert:
„da sollte der mephisto erkenntlich sein im top und halt beide züge“.
Erstellt und visuell geprüft: `first-esp-move/retro-mephisto-beide-zuege.png`,
480 x 480, Original-Retrostil, Kopf „Mephisto MM IV“, obere rote Zeile
E2 -> E4, untere rote Zeile E7 -> E5, eindeutige Spieler-/Enginebeschriftung.
Beide Züge stammen aus dem bestandenen ESP-Testlog; Darstellung ist gerendert.
Kopie auch im espmame-Produktprojekt unter docs/mame-first-move-20261110.

Optionalen Hardware-LCD-Build 16 gezielt beendet, damit die angeforderte Grafik
Vorrang hat. Der eigene PlatformIO-Prozess wurde vor dem Beenden per PID und
vollständiger Projekt-Kommandozeile verifiziert. Keine neue LCD-Firmware
geflasht. Build 16 hatte die neue C-Datei und retro_base.S bereits fehlerfrei
übersetzt, dann einen umfassenden SDK/MAME-Neubau ausgelöst; dieser ist
absichtlich unvollständig. LCD-Quellcode und CMake-Hooks sind als begonnene
Arbeit vorhanden, noch nicht am Gerät getestet. Das aktuell geflashte Abbild
ist weiterhin Build 13 (MM-IV/MM-V-Standalone, keine native Anzeige).

Offen für die Fortsetzung des MAME-Ports: Speicherfreigabe beim ROM-Wechsel,
Originalgeschwindigkeit (gemessen ca. 13-18 Prozent), Zusammenführung mit der
bestehenden Bauerino-Anwendung. Allgemeines Ausdünnen bleibt vertagt; echte
LCD-Anzeige hat der Nutzer mit der obigen Auswahl ebenfalls vorerst vertagt.

Nutzerwunsch für den ersten nachgewiesenen ESP-ROM-Zug: die echte Zugfolge und
Stellung in die Retrooberfläche übernehmen und davon einen Screenshot erstellen.
Keinen erfundenen Antwortzug als Emulationserfolg darstellen.

## Stabilitaetsstand 10.11.2026: Core v1

Nutzerauftrag: erst stabilisieren, danach MAME verschlanken fuer weitere CPUs.
Zwei gezielte Eigentums-/Freigabekorrekturen, keine allgemeine Funktionsentfernung:
- emumem.cpp-Overlay: Dispatch-Roots und verzogerte Handler-Referenzen freigeben.
- m6502.cpp-Overlay: lokales mi_default-Derivat gibt beide Zugriffscaches frei.
Originaldateien wurden dabei nicht editiert; CMake erzeugt lokale Kopien.

Build 22 / Flash 12 auf dem Waveshare bestanden. Sechs abwechselnde ROM-Starts
MM IV / MM V mit echten e2e4-Antworten e7e5 / c7c5, sauberen quits und
Speicherintegritaetspruefung. Nach Warmlaufen keinerlei Netto-Speicherverlust
zugelassen (--max-idle-loss 0). PSRAM nach allen Laeufen exakt 8.312.940 Bytes;
interner RAM pro Treiber stabil um 263.403 / 263.387 Bytes (letzter MM-V-Lauf
4 Bytes mehr frei, kein Verlust). Blockvergleich bestaetigt keine neuen Caches.

Anschliessend beide Treiber h2h4/le9/Speed0 jeweils 120 Sekunden nach go
rechnen lassen, dann quit: beide Ergebnis 0, Integritaet OK, keine
Allokations-/UCI-Fehler oder Panik und gleiche Speicherwerte nach Freigabe.
Logs takeover-load-cycle-03.log und takeover-search-stable-01.log.
Tempo unter Suche weiterhin etwa 14-15 Prozent des Originals.

Checkpoint backups/takeover-20261110/core-stable-v1 mit Firmware/ELF,
Bootloader/Partitionstabelle, Quellenarchiv, Testbelegen und SHA256-Manifest.
Details docs/16_stability.md. Es bleibt die Standalone-UCI-Testfirmware;
LCD/BLE/Wi-Fi-Produktintegration und Originaltempo sind noch offen.
Allgemeines Verschlanken und neue CPUs wurden in diesem Schritt nicht begonnen.
