# Gesicherter Stand und Weiterarbeit

Stand: 12. November 2026. Diese Datei ist der Einstieg für die nächste Sitzung.
Diese Sitzung endet mit der Sicherung; die untenstehenden Ideen sind keine
laufenden Aufträge.

## Aktiver, geprüfter Stand

- Zielhardware: Waveshare ESP32-S3-Touch-AMOLED-2.16, 480 × 480, 16 MiB Flash,
  8 MiB Octal-PSRAM, ESP-Takt 240 MHz.
- Aktuell auf dem Gerät: O2-Standard, zuletzt als Build 56 nach Flash 43 geprüft.
  Das vollständige Image ist bytegleich mit dem qualifizierten Build 52/53.
- Firmware-SHA-256:
  `e2e0b9cb03fd82b8d8a1b7ea7e18024af3c84d1f90bbc49e1cbc3a4d53a86cd5`.
- Letzter Quellcode-Commit vor dieser Dokumentationssicherung:
  `6e223e065da1f304a96301da5dc6bc16c1835cda`.
- MAME/MESS 0.220, derzeit MM IV und MM V mit 6502/65C02; Lua-/UCI-Brücke.
- Startanimation, Launcher, Virginia, Bots, Retro, WLAN-Portal und Weboberfläche
  bleiben in derselben Anwendung erhalten. Die letzte USB-Geräteregression
  bestätigt beide echten ROM-Züge e7e5/d2d4, Speicher-/SYNC-/Banktests,
  Oberflächenabläufe, saubere ROM-Beendigung und intakte Heaps.
- Physische Finger-/Handy-Bestätigung stammt aus früheren Nutzertests. Die
  jüngsten Prüfungen wurden über USB injiziert; keine neue physische Abnahme.

Beibehaltene Optimierungen: CPU-unity, übersprungene unverbundene SYNC-Aufrufe,
Lua-Auswahl-/LED-Caches, 17 CPU-Routinen und vier gemeinsame Speicherfunktionen
im IRAM, reduzierte Erzeugung ungenutzter DAC-Samples und direkte qualifizierte
Seitenzeiger mit Invalidierung bei Mapping-/Bankänderungen und nach State-Load.
RAM/ROM-Inhalte werden dabei nicht gespiegelt. Normale MAME-Fallbacks bleiben.

Anwendung: 5.006.971 Byte laut PlatformIO; statische Daten/BSS: 154.644 Byte.
Zusätzlicher gemeinsamer SRAM gegenüber Build 34: 3.088 Byte bei unverändertem
4.096-Byte-Budget. PSRAM-Speicherhelfer: 2.076 statt 6.168 Byte. Worker-Stack:
28 KiB. Original-ROM-Takte, Zyklen, Interrupts und Timer nicht verändern.

## Verlässliche Messwerte

Alle Geschwindigkeiten beziehen sich auf die bekannten 32-Halbzüge-Stellungen,
`le 9`, ungedrosselte Emulation, Original-ROM-Takt und 180 Sekunden Suchphase.
Vorbereitung/Einspielen der Züge zählt nicht zur Suchmessung.

| Messung | MM IV / Spanisches Zentrum | MM V / Drachenangriff |
|---|---:|---:|
| Qualifizierter O2-Stand 52/53 | 33,50874 % | 35,82792 % |
| O3-Kandidat 55 | 33,64835 % | 36,25524 % |
| Frische O2-Kontrolle 56 | 33,62484 % | nicht wiederholt |

O3 liegt bei MM IV damit nur 0,070 % relativ über der frischen Kontrolle.
MM Vs +1,193 % beziehen sich auf die ältere Referenz ohne neue Gegenmessung.
O3 benötigt zusätzlich 9.596 Byte Anwendung und 256 Byte SRAM. Deshalb bleibt
O2 Standard; O3 ist lediglich als `BAUERINO_MAME_CPU_OPT=O3` reproduzierbar.
Das Ziel von mindestens 50 % Originaltempo ist weiterhin offen.

Das aktuelle grobe Zeitprofil enthält je ROM 16 vollständige Intervalle und
etwa 80 Sekunden Auswertung ohne Lücken:

| Bereich | MM IV | MM V |
|---|---:|---:|
| CPU-Ausführung | 75,932 % | 75,056 % |
| Plattform/OSD inklusive Lua, Anzeige und Yield | 11,291 % | 12,246 % |
| Timer samt Rückrufen | 4,736 % | 4,778 % |
| Eingabeports | 3,245 % | 3,479 % |
| Scheduler | 2,290 % | 1,846 % |
| Audio | 1,482 % | 1,590 % |
| Video | 0,712 % | 0,772 % |

Das sind exklusive instrumentierte Wall-Zeiten einschließlich Unterbrechungen
und Messaufwand, keine reine CPU-Auslastung. Diagnosegeschwindigkeiten niemals
als Kandidatengewinn verwenden. Teilweise verschachtelte USB-Ausgaben können
Rate-Zeilen unbrauchbar machen; fehlende Werte nicht erfinden. Ein vollständiger
Emulator-Save/Load-Roundtrip ist trotz registriertem Postload-Schutz noch nicht
Teil der Geräteprüfung. Interne ROM-Brett-/Suchtiefen-Rücklesung fehlt ebenfalls.

## Ideen für die nächste Sitzung – noch nicht umgesetzt

1. **CPU-Kosten genauer eingrenzen.** Im aktuellen O2-Assembler Dispatch,
   Opcode-/Operanden-Fetch, virtuelle Speicheraufrufe und Flagberechnung
   untersuchen. Falls nötig begrenzte Diagnose oder Sampling einsetzen; keine
   dauerhaften Zähler pro Instruktion oder Speicherzugriff. Erst den konkreten
   Aufwand belegen, dann einen kleinen A/B-Kandidaten wählen.
2. **Häufige CPU-Pfade gezielt vereinfachen.** Die neuen direkten Seitenzeiger
   sind viel kompakter als bei früheren Versuchen. Prüfen, ob ein klar begrenzter
   gemeinsamer Fetch-/Zugriffspfad weitere Indirektionen einsparen kann, ohne
   großflächiges Inlining und IRAM-/Flash-Aufblähung. Das ist eine Hypothese,
   kein bereits gemessener Gewinn. Bank-/Mapping-/Tap-Fallbacks und echte
   CPU-Zyklen müssen identisch bleiben.
3. **Plattform-/Lua-Anbindung untersuchen.** Die 11–12 % sind nicht ausschließlich
   Lua. Bestehende Auswahl-/LED-Caches beibehalten. Ereignis-/Änderungsanzeigen
   könnten unnötige Abfragen vermeiden; vorher klären, welche Ausgangssignale
   und Multiplex-Zwischenstände die ROM-Brücke tatsächlich benötigt. Keine
   Sensor-, Tasten- oder LED-Ereignisse durch pauschales Polling-Auslassen verlieren.
4. **Timer und Ports erst danach.** Gezielte überflüssige Verwaltungsarbeit
   prüfen, aber keine ROM-Takte, Interruptfrequenzen, Emulationszeit oder
   Watchdog-/Idle-Yields reduzieren, um scheinbares Tempo zu erzeugen.
5. **Audio derzeit nachrangig.** Nur noch etwa 1,5 % gemessene Zeit. Weiteres
   pauschales Ausdünnen verspricht wenig und ist aktuell kein erster Kandidat.

Pro Versuch nur eine wesentliche Änderung, tatsächliches SRAM inklusive
IRAM/Literalen aus der Map prüfen, beide ROMs funktional testen, anschließend
dieselben Mittelspielstellungen ohne Diagnose messen. Bei kleinen Differenzen
eine frische Kontrolle einplanen. Erst danach Standard übernehmen oder den
bewährten Stand wiederherstellen. Keine Geschwindigkeit vorab versprechen.

## Nicht blind wiederholen

- Ganze CPU-Speicherpfade inline/direkt (32/33): vorher langsamer.
- Begrenztes Hot-Inlining (39): kleiner Gewinn bei 1.040 zusätzlichen SRAM-Byte;
  nicht übernommen. Neuer Versuch nur mit klarer Abgrenzung zur heutigen
  direkten Seitenstruktur.
- Weglassen der letzten Stereo-Konvertierung (42): praktisch kein Gewinn.
- Low-page-Metadaten im SRAM (49): ungefähr 9,5 % langsamer bei beiden ROMs.
- CPU-O3 (55): gegenüber frischer MM-IV-Kontrolle praktisch kein Unterschied.
- Gesamtes CPU-Objekt ins interne RAM: früher kein überzeugender Nutzen.

## Sicherungen und Einstiegspunkte

Privat/lokal, nicht ins öffentliche Git aufnehmen:

- `backups/takeover-20261110/combined-build56-o2-control/`: aktuelles Image,
  ELF/Map, Bootloader, Partitionstabelle, Dateisystem, O2-Assembler,
  Quellcode-Abzug, Messprotokolle und `checkpoint.json` mit SHA-256-Dateiliste.
- `combined-build53-flat-default/`: ursprünglicher qualifizierter Standard.
- `combined-build54-current-core-profile/`: aktuelle Diagnose.
- `combined-build55-cpu-o3/`: reproduzierbarer O3-Kandidat.
- Unverändertes Vollbackup vor MAME:
  `backups/takeover-20261110/waveshare-before-mame-16MB.bin`, SHA-256
  `4264eb20d86cfd878d9135853a6b27c1ec817379fa63fbfc15245250c38a8e1e`.
- ROM-Image: `integration/chessengine/build-host/rom-image/roms-fat-wl.bin`,
  SHA-256 `0fdaa45b5d434dd361fd94dc61f5b21efc538004c7ef58536ecebb100d7790e0`.
  ROMs, Vollbackups und Firmware-Binaries nicht veröffentlichen.

Quellcode: `bauerino/mame_build.py`, `tools/prepare_65xx_speed.py`,
`tools/prepare_flat_banks.py`, `esp32s3/components/mame0220/src/s3_fast_memory.h`.
Tests: `tools/test_bauerino_device.py`, `tools/benchmark_retro_middlegame.py`,
`tools/report_core_profile.py`, `tools/verify_hot_routines.py`.
Details: [direkte Seiten](40_flat_page_pointers.md),
[aktuelles Profil und O3-Vergleich](41_current_profile_cpu_optimization.md).

Workspace ist der Elternordner dieses Repositories. Python/PlatformIO liegen
unter `output/bauerino-pio-core/penv/Scripts/`. PowerShell ohne Login-Profil
verwenden; System-Python-Alias funktioniert hier nicht zuverlässig. Builds
benötigen oft 7–10 Minuten. Während eines Builds keine Build-Eingaben ändern.
USB-Gerät zuletzt COM8, VID/PID 303A:1001, Seriennummer `44:BD:8D:60:CB:44`;
Identität vor Flashen über das vorhandene Guard-Tool prüfen, nicht nur COM8
vertrauen. USB muss erst beim nächsten Gerätetest wieder verfügbar sein.

Nächste freie Kennungen: Build 57, Flash 44, Geräteregression 57,
Mittelspielmessung 39. Alte nummerierte Verzeichnisse nicht überschreiben.
Keine Aufgabe oder Messung läuft nach Abschluss dieser Sicherung weiter.

Späterer Produkt-Backlog: DE/EN-Umschalter im Setup; vollständiges natives
Retro-Spiel mit physischem Chessnut/Chesslink separat qualifizieren; weitere
ROMs/CPU-Familien nach stabiler, ausreichend schneller Basis. MAME weiter
verschlanken bleibt ein eigenes Thema. Ein kompletter MAME-Ersatz ist nicht
beschlossen.
