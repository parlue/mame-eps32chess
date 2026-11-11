# ESP-UCI-Brücke für MessChess / MM IV und MM V

Stand: 10. November 2026. Eigenständiges Integrationsmodul außerhalb des aktiven
ESP-IDF-Baums. Der laufende MAME-Build, dessen Konfiguration, Startdateien und
Referenzquellen werden durch dieses Modul nicht geändert.

Die Brücke portiert das lokale `MessChess/plugins/chessengine/init.lua`
(Version 2024.1.1) und die Interfaces `mm4.lua` / `mm5.lua`. Sie verwendet
MAMEs vorhandene Lua-5.3.4-Quellen, aber **weder das MAME-Lua-Frontend noch
sol2, LuaFileSystem, Windows-Prozesse oder einen blockierenden stdin-Thread**.

## Enthalten

- Echter UCI-Parser und Zugübersetzung aus dem lokalen Plugin; MM-V-Interface
  lädt wie im Original das MM-IV-Interface.
- `retro::Runtime`: kooperative Lua-Coroutines, gepufferte UCI-Zeilen,
  Eingabefeld-/Output-Brücke, Softreset, Geschwindigkeit und Maschinenende.
- `retro::MameMachine`: Adapter für die öffentlichen APIs des vorhandenen
  MAME-0.220-Quellbaums (`root_device().ioport`, `output`, `video`, Reset/Exit).
- Originale Behandlung von Rochade, Schlagzügen, en passant, Umwandlung und
  Rückmeldung des Computerzuges über LEDs / sieben-Segment-Ausgaben.
- Lua-Allokationen auf dem ESP in PSRAM, maximal 768 KiB; 32 Zeilen und
  höchstens 8 KiB Text je Ein-/Ausgangsqueue, maximal 4096 Bytes je Zeile.
- Abbruch schließt die VM und lässt gehaltene Tasten los. `quit` hat Vorrang
  vor wartenden Befehlen und beendet die Maschine, nicht den ESP.
- Ein Instruktionslimit gilt pro Coroutine-Fortsetzung. Keine blockierenden
  Wartezeiten: `poll()` kehrt während emulierter Tastendrücke sofort zurück.

Die generierten Skripte samt Bootstrap brauchen rund **33 KB**. Das ist nur der
Skriptanteil, nicht der gesamte Lua-Interpreter oder der MAME-Kern.

## Lokal erzeugen und prüfen

```powershell
python tools/import_cbemu.py --source C:/Users/DirkSommerfeld/Documents/CB-Emu
powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_host.ps1
python tools/check_xtensa.py
python tools/prepare_roms.py --cbemu C:/Users/DirkSommerfeld/Documents/CB-Emu --output local-roms --machine mm4 --machine mm5
```

Aus dem Modulverzeichnis ausführen. Python und Visual Studio Build Tools sind
für die Hostprüfung nötig. `check_xtensa.py` liest die vorhandene
`compile_commands.json` und erzeugt ausschließlich in `build-xtensa/` ein
separates `libretro_uci.a`. Es startet keinen PlatformIO-/MAME-Neubau.

Die ROM-Vorbereitung prüft Größe, CRC32 und SHA1. Sie erzeugt:

| Modell | Datei unter VFS-ROM-Pfad | Größe | CRC32 |
|---|---|---:|---|
| MM IV | `mm4/710` | 32768 B | f68a4124 |
| MM V | `mm5/mephisto5.rom` | 32768 B | 89c3d9d2 |

Die MM-V-Datei heißt in CB-Emu `mm5.bin`. Ihr Inhalt entspricht dem Original-
0.220-Treiber; der Import benennt die private lokale Kopie passend um.
Beide vorbereiteten Verzeichnisse wurden mit dem vorhandenen offiziellen
`mame64.exe` (0.220) und `-verifyroms` erfolgreich geprüft.

`local-roms/` ist kein gemountetes ESP-Dateisystem. Die Dateien müssen später
in das tatsächlich gemountete VFS unter `/roms/mm4/710` bzw.
`/roms/mm5/mephisto5.rom` übertragen werden. Die aktive Firmware/Partition
wurde weder geändert noch geflasht. Keine ROMs sind im Skript-Payload eingebettet.

## Nach Abschluss des laufenden Kern-Builds anschließen

1. Diesen Ordner als `EXTRA_COMPONENT_DIRS` in das IDF-Projekt aufnehmen,
   bevor dessen `project()` ausgewertet wird. MAME-Komponente benötigt dann
   `REQUIRES chessengine`, um das Runtime-Modul zu verwenden.
2. ROM-VFS mounten, ROM-Dateien bereitstellen, `rompath=/roms` setzen.
3. Modell **vor** dem Aufbau von `machine_config` auswählen. Der vorhandene
   Startpfad enthält bisher nur `mm4` in seiner Treibertabelle. Für MM V muss
   dort auch `GAME_EXTERN(mm5)` / `GAME_NAME(mm5)` eingetragen werden.
4. Nach dem Maschinenstart `MameMachine` und `Runtime` erzeugen. Im regelmäßigen
   MAME-/OSD-Callback auf demselben Task `poll(machine.time().as_double())`
   aufrufen. Die Emulation muss zwischen den Aufrufen weiterlaufen.
5. UI-/Brett-Task über begrenzte FreeRTOS-Queues anbinden. **Keine Runtime-
   Methode von mehreren Tasks gleichzeitig aufrufen.** Gerätezugriffe gehören
   ausschließlich auf den MAME-Task. Eingänge vor `poll()` mit `send()`
   zuführen, anschließend Antworten mit `read()` abholen.
6. Bei Sessionwechsel alte Transportzeilen verwerfen, Runtime schließen und
   die Maschine für eine neue Partie zurücksetzen. `error()` an den vorhandenen
   Partieabbruch weiterreichen; die VM nie länger als die Maschine leben lassen.

Minimale Verwendung auf dem MAME-Task:

```cpp
#include "retro_mame_machine.h"
retro::MameMachine adapter(machine);
retro::Runtime uci(adapter);
if (!uci.open()) { /* uci.error() melden */ }
// Im regelmäßigen Callback, nicht in einer blockierenden Warteschleife:
// while (frontendQueue.read(line)) uci.send(line);
// uci.poll(machine.time().as_double());
// while (uci.read(line)) frontendQueue.reply(line);
```

Das ist der Anschlussvertrag, noch keine Änderung von `s3_osd.cpp` oder
`s3_startup.cpp`. Die echte Verbindung mit Kernel, ROM, Display und Brett
folgt nach dem laufenden Link-/Startnachweis.

## UCI-Unterschiede gegenüber Virginia

Das Plugin lädt das ROM **nicht** mit `setoption`. Startparameter/Profil wählen
zuerst den MAME-Treiber, optional ein Cartridge, und damit das Lua-Interface.
Erst danach kommt UCI:

```text
uci
  id name Mephisto MM IV ...
  option name Speed type spin default 100 min 0 max 10000
  option name Level type string default le 1
  uciok
setoption name Level value le 3
ucinewgame
isready
  readyok
position startpos moves e2e4
go
  bestmove e7e5
```

Antwortzug dient hier nur als Protokollbeispiel. `Level` wird über echte emulierte
LEV-/Ziffer-/ENT-Tastendrücke gesetzt; `Speed` ist ein Geschwindigkeitsfaktor
(100 = 100 %, 0 = ungebremst), keine Änderung des originalen CPU-Taktes.
MM-IV-Werte: `le 0` bis `le 9`, `bl 1` bis `bl 8`.

Der Originalparser wertet `go movetime`, Tiefe und UCI-Uhrparameter nicht aus.
Die Engine denkt gemäß ihrem eingestellten Geräte-Level. Der Frontend-Timeout
`moveMs + 15000` muss deshalb für Retro ersetzt werden; ein langsamer Retrozug
darf keine automatische Fehlermeldung auslösen. `isready` wird erst nach den
laufenden Tastensequenzen beantwortet, deshalb auch die Bereitschaftsfrist
großzügig und getrennt vom Suchlimit behandeln.

Der Originalcode meldet keine standardisierten UCI-Werte für Tiefe, Bewertung,
PV oder beide Uhren. Für die Retro-Doppelanzeige müssen später gezielt
gerätespezifische Outputs/RAM gelesen und separat an die UI gegeben werden.

## Erste Version: bewusst begrenzter Modellumfang

- MM IV und MM V, jeweils Standardsatz; keine weiteren 462 Profile aktiviert.
- FEN-Setup wird ausdrücklich abgewiesen. Der Original-FEN-Pfad erhält nicht
  alle Rochade-/en-passant-Rechte; für den Ersttest genügt Startstellung plus Züge.
- `stop` verwendet die jeweilige Original-Gerätefunktion und garantiert nicht
  sofort ein `bestmove`. Zum Abbruch Runtime schließen/`quit` verwenden.
- Keine Save-State-Übernahme vom PC. NVRAM-/Konfigurationspersistenz und
  optionale Buchmodule (HG440/HG550, jeweils 16 KiB) sind spätere Schritte.
- Original-ROM-Leistung und Gesamtverbrauch mit Display, BLE und Wi-Fi wurden
  noch nicht auf dem ESP gemessen. Die Brücke allein beweist keine spielbare
  MAME-Emulation auf dem Gerät.

## Prüfungen

Hosttests führen den **wirklichen importierten Lua-Code mit Lua 5.3.4** aus,
gegen nachgebildete Geräteein-/ausgänge: Handshake, Level-/Speed-Optionen,
Weiß-/Schwarz-Eröffnung, LED→bestmove, Rochade, en passant, Unterverwandlung,
Reset, Abbruch mit gedrückter Taste, Sessionneustart, defekte Befehle und volle
Queues. Das ersetzt keinen ROM-Lauf. Gemessener VM-Heap am PC nach dem
MM-IV-Handshake liegt bei rund 207 KB; kein ESP-RAM-Messwert.

Die Kernel-/Geräte-API wird zusätzlich mit dem vorhandenen S3-Compiler gegen
die tatsächlichen MAME-0.220-Header geprüft. ESP-Objektcode und Hostprotokoll
werden separat nachgewiesen, nicht als fertige Gesamtfirmware bezeichnet.
