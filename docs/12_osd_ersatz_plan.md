# 12 – Plan Meilenstein 3: OSD-Ersatzschicht für ESP-IDF

Erzeugt: 2026-11-09. Grundlage: die Schnittstellen, die MAME 0.220 verlangt –
`src/osd/osdepend.h` und `src/osd/osdcore.h`.
Dies ist ein **Plan**; Nachweise folgen in `docs/10_build_log.txt`.

## Umsetzungsstand (Lauf #22): die Schicht existiert

Anders als in der ersten Fassung dieses Dokuments angenommen, ist die
OSD-Schicht **nicht** erst Meilenstein 3: `running_machine::start()` ruft
`manager().osd().init(*this)` und in jedem Frame `osd().update()` /
`osd().input_update()` auf. Ohne die Schicht gibt es also **kein** lauffähiges
Programm, sondern nur eine Bibliothek. Sie ist deshalb zusammen mit M1b/M1c
entstanden. Diese Dateien setzen sie um (alle unter
`components/mame0220/src/`, nichts davon liegt im MAME-Originalbaum):

| Datei | Inhalt |
|---|---|
| `s3_osd.h` / `s3_osd.cpp` | `class s3_osd : public osd_interface` – alle 16 Methoden aus `osdepend.h`, überwiegend wirkungslos. `update()` gibt mit `vTaskDelay(1)` den Kontrollfluss an FreeRTOS zurück (sonst verhungert der Leerlauf-Task und der Task-Watchdog schlägt zu) |
| `s3_osd_core.cpp` | alle freien Funktionen aus `osdcore.h`: `osd_file` (VFS/stdio, offset-basiert), `osd::directory` (opendir/readdir), `osd_ticks`/`osd_ticks_per_second`/`osd_sleep` (esp_timer, 1 Tick = 1 µs), `osd_stat`, `osd_work_*` (Arbeit läuft sofort im aufrufenden Kontext, kein Threading), `osd_uchar_from_osdchar` (UTF-8), Pfad-/Dateinamenprüfungen, `osd_getenv`, `osd_break_into_debugger` |
| `s3_startup.cpp` | Treibertabelle (Ersatz für die erzeugte `drivlist.cpp`: enthält genau `mm4`), `class s3_machine_manager : public machine_manager`, Ausgabeumleitung (`osd_output` → UART), Einstieg `mame_s3_run()` |

Was MAME sonst selbst liefert und hier ersetzt wird:

* **`drivlist.cpp`** – MAME erzeugt die Tabelle aller Treiber aus `mame.lst`.
  Unser Bau enthält genau einen Treiber; die Tabelle `driver_list::s_drivers_sorted`
  wird deshalb in `s3_startup.cpp` gesetzt (`GAME_EXTERN(mm4)` / `GAME_NAME(mm4)`
  = `driver_mm4`, das Symbol kommt unverändert aus `mephisto_mm2.cpp`).
* **`version.cpp`** – liefert `build_version`/`bare_build_version`; beide sind in
  `s3_startup.cpp` definiert.
* **`src/emu/main.cpp`** – MAMEs `main()`; auf dem S3 ist `app_main()` der
  Einstieg (siehe `src/main.c`), es ruft `mame_s3_run()`.
* **`src/frontend/mame/*`** – Menüs und Kommandozeile. Werden nicht gebraucht:
  `machine_manager::create_ui()` liefert laut `src/emu/main.h` von sich aus
  `nullptr`, und `running_machine::ui()` (mit `assert(m_ui != nullptr)`) wird nur
  von der Menüebene aufgerufen.
* **Layoutdateien des Treibers** – `mephisto_mm2.cpp` bindet `mephisto_mm2.lh`
  und `mephisto_bup.lh` ein, die MAME zur Bauzeit mit `scripts/build/complay.py`
  aus `src/mame/layout/*.lay` erzeugt. Dieses Originalwerkzeug wird im
  CMake-Schritt aufgerufen (`add_custom_command`), damit der Treiber
  unverändert bleibt und kein Fork nötig ist.

## Warum überhaupt

MAME trennt Kern und Betriebssystem über die OSD-Schicht. Auf dem S3 gibt es
weder SDL noch Windows/DOS – die Schicht wird also **vollständig ersetzt**, nicht
portiert. Alles, was nur für Grafik/Ton/Fenster/Debugger gebraucht wird, bleibt
leer. Gemessen wird später mit `nm firmware.elf`: es dürfen keine **undefinierten**
`osd_*`-Symbole übrig bleiben.

## 1. `osd_interface` (osdepend.h:60–98)

Alle Methoden sind rein virtuell, es gibt also keinen Standard. Plan je Methode:

| Methode | Umsetzung auf dem S3 |
|---|---|
| `init(running_machine&)` | Emulation vorbereiten, Startzeit merken; Rückruf zum Start des Emulationsloops |
| `update(bool skip_redraw)` | Taktgeber des Loops (Ausgabe/Zeit); auf dem S3 von unserem FreeRTOS-Task aufgerufen |
| `input_update()` | Tastaturmatrix aus der UART-Seite einspeisen (`KEY.0`/`KEY.1`) |
| `set_verbose(bool)` | Logstufe für `osd_printf_*` |
| `init_debugger()`, `wait_for_debugger(...)` | leer (kein Debugger im Gerät) |
| `update_audio_stream(...)`, `set_mastervolume(int)` | leer; `no_sound()` → `true` |
| `customize_input_type_list(...)` | leer (MAME-Standardbelegung genügt) |
| `add_audio_to_recording(...)` | leer |
| `get_slider_list()` | leere Liste |
| `font_alloc()` | `nullptr` (keine Schriftdarstellung) |
| `get_font_families(...)` | `false` |
| `execute_command(const char*)` | `false` |
| `create_midi_device()` | `nullptr` |

## 2. `osd_file` (osdcore.h)

Statisch: `open(path, openflags, file, filesize)`, `openpty(...)`, `remove(name)`.
Rein virtuell (genau vier, nachgeprüft in osdcore.h): `read(buffer, offset,
length, actual)`, `write(buffer, offset, length, actual)`, `truncate(offset)`,
`flush()`. Ein `close()` gibt es nicht – geschlossen wird im Destruktor.

Umsetzung: offset-basierte Dateioperationen über `stdio` des ESP-IDF-VFS
(FAT-Partition `roms`, siehe `partitions.csv`). `openpty` → Fehler (keine
Pseudoterminals). `truncate` leert nur den Puffer (kein `ftruncate` im VFS).

## 3. `osd::directory` (osdcore.h:308–366)

`osd::directory::open(dirname)` statisch, `read()` liefert Einträge
(`name`, `type` FILE/DIR/OTHER, `size`, `last_modified`). Wird für die
ROM-Suche gebraucht: Die ROM-Dateien liegen als Verzeichnis auf der
`roms`-Partition (`/roms/mm4/710`).

## 4. Freie OS-Funktionen (osdcore.h:209–410 ff.)

`osd_getenv`, `osd_getpid`, `osd_get_physical_drive_geometry` (immer `false`),
`osd_uchar_from_osdchar`, `osd_is_valid_filename_char`,
`osd_is_valid_filepath_char`, `osd_is_absolute_path`, `osd_ticks` /
`osd_ticks_per_second` (über `esp_timer_get_time`), Schlaffunktion, die
`osd_printf_*`-Familie (Ausgabe über UART) sowie Dateisystemhilfen
(`mkdir`/`rmdir`/Arbeitsverzeichnis). Die genaue Liste wird beim ersten
Linkversuch aus den **undefinierten Symbolen** abgeleitet, nicht geraten.

## 5. Was ausdrücklich nicht gebaut wird

Grafik (bgfx/SDL/HLSL), Tonausgabe, Schriftdarstellung, MIDI, Debugger-UI,
Lua-Plugin-System. Damit entfallen auch die größten Teile des MAME-Frontends
(`src/frontend`), die in `src/emu` nur über `osdepend.h:19` anklingen.
