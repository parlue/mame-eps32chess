# Was der MAME-Port für MessChess noch bereitstellen muss

Stand: 10. November 2026. Prüfung des lokalen MAME-0.220-Quellbaums und
`esp32s3/components/mame0220/CMakeLists.txt`. Der laufende Build wurde nicht
verändert. Die Modellliste daneben beruht auf dem Original-PC-Programm;
sie ist kein ESP-Laufnachweis.

## Unmittelbar für MM IV / MM V

1. **Vollständige Geräteabhängigkeiten bauen und linken.**
   `src/mame/machine/mmboard.cpp` ist vorhanden und im Bauplan. Diese
   Mephisto-spezifische Klasse verwendet zusätzlich die eigenständigen Typen
   `SENSORBOARD` und `PWM_DISPLAY`. Ihre Definitionen liegen in
   `src/devices/machine/sensorboard.cpp:62` und
   `src/devices/video/pwm.cpp:40`. Beide Quelldateien fehlen im untersuchten
   CMake-Bauplan. Deshalb nicht `mmboard.cpp` ersetzen, sondern die beiden
   generischen Implementierungen zusätzlich aufnehmen, sofern sie nicht
   inzwischen aus einer anderen echten Bibliothek hinzugelinkt werden.
   Das betrifft Geräte-Logik auch bei abgeschalteter grafischer Darstellung.

2. **Treiberauswahl.** Zwei CPU-Gerätetypen sind vorhanden: `M6502` und `M65C02`.
   Die Dateien mit `d` im Namen sind Disassembler. Die Starttabelle registriert
   bisher nur `mm4`. Bereits `mephisto_mm2.cpp` enthält 16 passende Set-IDs;
   vor dem Erzeugen von `machine_config` Treiberwahl erlauben und die gewünschten
   Einträge in `driver_list` aufnehmen. MM V benötigt keinen neuen CPU-Kern.

3. **ROM-Dateisystem vor Maschinenstart mounten.** Der Startpfad verwendet
   `/roms`, das allein noch kein gemountetes Dateisystem ist. Datei-I/O,
   Verzeichnisse, Suchen und echte Hashprüfung müssen funktionieren. Zum
   Ersttest genügen lose Dateien `/roms/mm4/710` und
   `/roms/mm5/mephisto5.rom`; ZIP-Import ist zusätzlich möglich, wenn die
   Archive-/Dekompressionsabhängigkeiten vollständig eingebunden sind.
   Beide privaten ROM-Sätze sind in `local-roms/` vorbereitet und geprüft.

4. **Echte Ein-/Ausgaben zugänglich machen.** Feldzugriff über
   `machine.root_device().ioport(tag)->field(mask)->set_value(value)` und
   Outputwerte über `machine.output().get_value(name)`. Bei MM IV/V gehören
   dazu `:KEY1_*`, `:KEY2_*`, `:board:board:RANK.*`, `:board:board:UI`,
   `:board:board:SPAWN`, `digit0..3`, `led0..63` und `piece_ui0`.
   Keine Platzhalterwerte: daraus entstehen UCI-Züge und Umwandlungen.

5. **Regelmäßigen Callback auf dem Emulationstask.** Die Brücke muss während
   des CPU-Laufs weiter gepollt werden; emulierte Zeit als monotone Sekunden
   bereitstellen. Tastendrücke warten auf emulierte Zeit, ohne UI/BLE/Wi-Fi zu
   blockieren. Geschwindigkeitsregelung erhalten, nicht nur im Scheduler blind
   frei laufen lassen. `Speed=100` bedeutet Originalgeschwindigkeit;
   tatsächliche Erreichbarkeit ist zu messen.

6. **Reset, Abbruch und Lebensdauer.** Softreset für neue Partie, Maschinenende
   ohne ESP-Neustart, alle Eingaben bei Abbruch freigeben. Alte UCI-Antworten
   dürfen keine neue Sitzung erreichen. UI und MAME über begrenzte Queues
   koppeln; Gerätezugriffe nur auf dem MAME-Task. Beim Modellwechsel die alte
   Maschine sauber abbauen und ROM/RAM freigeben.

7. **UCI-/Lua-Modul anschließen.** Das neue Modul in diesem Verzeichnis enthält
   die erste portierte Steuerung für MM IV/V, mit Lua 5.3.4 und kleiner eigener
   API-Brücke. Volles MAME-Lua-Frontend, sol2, LuaFileSystem und Windows-Threads
   sind dafür nicht erforderlich. Hosttests und separate S3-Kompilierung sind
   bestanden; mit dem aktiven Kernel ist es noch nicht verbunden.

## Für weitere MessChess-Geräte

8. **Modelltreiber und zusätzliche Bausteine selektiv ergänzen.** Die Liste
   `audit/65xx-modelle.md` nennt alle geprüften Set-IDs und pro Treiber die
   zusätzlichen Gerätetypen. Polgar/Milano/Academy benötigen z. B. emulierte
   LCD-Hardware einschließlich HD44780 und dessen Zeichen-ROM. Novag
   Constellation benötigt den Beeper-Baustein. Diese Geräte können für
   Speicher-/Outputabfragen nötig sein, auch wenn das originale Artwork nicht
   gezeichnet wird. Nicht pauschal alles aus MAME übersetzen.

9. **Weitere Lua-Interfaces und API-Funktionen.** Bisher nur `mm4`/`mm5`
   portiert. Andere Interfaces benötigen u. a. `emu.item`, Geräte-RAM-Zugriffe,
   LCD-Speicher oder eigene Promotionsfunktionen. Je Modell Abhängigkeiten
   einschließlich `load_interface(...)` und Parent-Fallback übernehmen.

10. **Softwaremodule/Cartridges.** Für ARB/GGM wird der Basis-Treiber zusammen
    mit einem Modul gestartet (`arb sargon25`, `ggm morphy` usw.). Benötigt
    Cartridge-Gerät, Softwareliste, ROM-Banking und Modellprofil. Ein bestandener
    Basis-ROM-Audit bestätigt noch kein eingelegtes Modul. Optionale
    Eröffnungsbücher (HG440/HG550 usw.) sind ebenfalls Medien, keine UCI-ROM-Option.

11. **Versionen unterscheiden.** Einige CB-Emu-Set-IDs sind im Original-0.220
    anders benannt oder fehlen. Erst ROM-Hashes und Hardware abgleichen;
    danach Originalprofil zuordnen oder gezielte Treiberänderungen portieren.
    Rockwell `R65C02`, `M65SC02`, MOS `M6504` usw. sind nicht durch die beiden
    vorhandenen CPU-Typen automatisch abgedeckt. Weitere CPU-Architekturen
    sind nur für ausdrücklich gewünschte spätere Modelle erforderlich.

## Anpassungen an Bauerino, außerhalb des MAME-Kerns

12. Retro als nativen Transport an die vorhandene UCI-Schicht anbinden. Die
    aktuelle MAME-Auswahl im Frontend verwendet noch TCP; das Display zeigt
    bisher eine Retro-Vorschau. `go movetime` wird vom MessChess-Plugin nicht
    als Zeitbudget ausgewertet. Geräte-`Level` benutzen und den bisherigen
    Suchabbruch nach `moveMs+15000` für Retro ersetzen. Bereitschaft kann
    mehrere emulierte Tastendrücke benötigen. Optionen pro Modell speichern.

13. Die Retro-Uhren und Anzeigen benötigen gesondert gelesene Gerätewerte.
    Standard-UCI dieses Plugins liefert weder beide Uhren noch Tiefe, PV und
    Bewertung. Fehlende Werte als unbekannt anzeigen. NVRAM und Einstellungen
    gezielt persistent machen; PC-Save-States nicht ungeprüft übernehmen.

14. Für die Gesamtfirmware **eine gemeinsame Toolchain/SDK-Konfiguration**
    verwenden. Der untersuchte MAME-Port nutzt ESP-IDF 5.3; Bauerino nutzt
    Arduino 3.3.5 auf IDF 5.5. Das getrennt geprüfte Archiv ist kein Beweis,
    dass beliebige vorgebaute Bibliotheken beider SDKs zusammenpassen.
    Beim Zusammenführen alle beteiligten Quellen passend neu bauen.

15. PSRAM für große MAME-/Lua-Allokationen vorsehen; internen Heap für
    Scheduler, BLE, Wi-Fi und Display-Transfers freihalten. Pro Maschine
    tatsächlichen Spitzenverbrauch und Emulationsrate messen. Ersttest:
    ROM bootet → UCI bereit → legale Partie mit beiden Farben → Sonderzüge →
    Abbruch/Neustart → Betrieb mit Chessnut/ChessLink und Display.

## Kurze Ergänzung für den parallel laufenden MAME-Build

> Bitte zusätzlich zu `src/mame/machine/mmboard.cpp` dessen transitive
> Geräteabhängigkeiten prüfen: `SENSORBOARD` wird in
> `src/devices/machine/sensorboard.cpp` definiert und `PWM_DISPLAY` in
> `src/devices/video/pwm.cpp`. Beide werden von `mmboard.cpp` instanziiert.
> Sicherstellen, dass die echten Implementierungen im Build und Link enthalten
> sind; Nachweis über Objektdateien und Link-/Symbolprüfung. Keine Stubs und
> kein Austausch von `mmboard.cpp`.
