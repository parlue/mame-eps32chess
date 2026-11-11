# Retro-Testoberflaeche und erster Schlankheitsschritt

Auftrag: zuerst den stabilen ROM-Kern am Waveshare bedienbar machen, danach
gezielt verschlanken, um Platz fuer weitere CPU-Familien zu gewinnen.

## Bedienbare native Testversion

Die Gesamtanwendung espmame verwendet Arduino/IDF 5.5; der MAME-Testkern IDF
5.3. Die bisherige Display-App hat einen 460.800-Byte-Bildpuffer. Im aktiven
MAME-Stand sind nur etwa 130 KiB PSRAM frei. Deshalb wurde zunaechst eine native
Testoberflaeche mit nur einer 960-Byte-Pixelzeile angebunden. Die bisherige
Gesamt-App und ihr vollstaendiges Flash-Backup bleiben erhalten.

- CO5300 480x480 mit Original-Retrohintergrund und Original-Schachfiguren im
  Flash; kein Vollbildpuffer. Brettfelder sind quadratisch (44x44 Pixel).
- CST9220 ueber IDF-I2C, Pins/Koordinatentransformation wie in Bauerino.
- Acht Funktionen: Board, Notation, Load, Options, Play, Move, Info, Quit.
- Load waehlt MM IV oder MM V und beendet bei Bedarf die vorherige Maschine.
- Options stammen aus echten UCI-option-Zeilen (aktuell Speed und Level).
  Eingabe am Display; Werte gelten beim naechsten Play. Level-Eingaben sind
  gegen die vorhandene MM-IV/MM-V-Lua-Schnittstelle begrenzt.
- Play/Weiss startet einen gekennzeichneten e2-e4-Test ohne physisches Brett;
  Play/Schwarz laesst das ROM mit Weiss beginnen. Kein wieder eingefuehrter
  Touch-Play-Modus. Move fordert den naechsten ROM-Zug an und wechselt damit
  die von der Engine gespielte Farbe. Testlaenge maximal 128 Halbzuege.
- Board/Notation folgen echten UCI-Zuegen, einschliesslich Rochade,
  en passant und Umwandlung. Die GUI ist kein eigener Schachregelpruefer.
- Info zeigt gemessenes Tempo; keine erfundenen depth/score/PV-Werte.
- Keine BLE-/Wi-Fi- oder Virginia/Bots-Gesamtintegration in dieser Testversion.

Hosttest `tests/retro_ui_test.c` bestanden: echte UI-Tap-Routen und ausgehende
UCI-Kommandos fuer Discovery, Farbwahl, Move, Board/Notation, Quit und Wechsel.
Build 24 erfolgreich: 9.775.016 Byte Flash, 68.628 Byte statisches RAM.
Flash 13 und automatisierter Geraetetest bestanden: e2e4 -> e7e5 (MM IV),
Move -> g1f3 (MM IV mit Weiss), MM V eroeffnet d2d4. Beide sauberen Exits
mit intaktem Heap, PSRAM jeweils 8.311.916 Byte frei.

Die ersten USB-LCD-Abzuege waren durch CRLF-Konvertierung der Binaerdaten
beschaedigt und sind KEINE freigegebenen Screenshots. Build 25 schaltet fuer
den Capture-Vorgang die VFS-Ausgabe auf unveraenderte LF-Bytes. Neupruefung
und physische Fingerbestaetigung stehen an.

### Physische Rueckmeldung und laufende Korrekturen

Dirk sah zunaechst eine Anzeige, aber keine Reaktion auf Load. Native Touch-
Anbindung korrigiert (Build 26/Flash 15): CST92xx hat ZWEI Finger und damit
15 Byte Nachricht, nicht 30. STOP zwischen I2C-Adressierung und Lesen wie im
Arduino-Original; Normalmodus, fallende IRQ-Flanke, Originaltransformation.
Reale Fingerereignisse in takeover-touch-01.log (keine eingespritzten Taps):
Load bei 288/329 und MM IV bei 65/162, danach `[LOAD] mm4 (touch)` und UCI.

Dirk meldete danach bei USB-Trennen/Wiederverbinden ein dunkles Display.
Auch ein kontrollierter USB-Reset mit vollstaendig erfolgreichem Firmwareboot,
LCD-/Touch-Init und fehlerfreiem Softwarebild (`retro-boot-26/00-retro-start.png`)
liess laut Dirk das physische Panel dunkel. Damit ist physische Anzeige noch
NICHT bestaetigt. Transport-Erfolg und Softwarebild beweisen keine sichtbaren
Pixel. Fruehere sichtbare Inhalte koennten vom zuvor initialisierten Panel
stammen; das ist bislang eine Hypothese.

Build 27 gleicht QSPI an Arduino_ESP32QSPI an: CS12 manuell statt Hardware-CS,
Bus dauerhaft reserviert, GPIO-Matrix explizit, Reset low/high jeweils 200 ms
wie CO5300-Originaltreiber. Build/Flash erfolgreich; Nutzer meldet weiterhin dunkel.
Build 28 richtet Fenster auf gerade/ungerade Zeilenpaare aus und haelt CS
ueber beide Zeilen. Build erfolgreich (85.83 s), Flash 17 Hash verifiziert.
USB-Boot/Capture-Test erfolgreich; physische Sichtbarkeit weiter zu bestaetigen.

Build 29 korrigiert zusaetzlich die GPIO-Funktionsauswahl fuer LCD-Reset39,
CS12 und Touch-Reset40 durch gpio_config. Lokaler IDF-5.3-Quellvergleich:
gpio_set_direction setzt Richtung/Output-Matrix, jedoch keine IO-Mux-Funktion;
gpio_config waehlt explizit PIN_FUNC_GPIO. GPIO39 besitzt auch Funktion MTCK.
Dies ist ein konkreter Initialisierungsunterschied zur Arduino-pinMode-Referenz,
noch kein Nachweis fuer die Ursache des dunklen Panels.
Build 29 erfolgreich (92.41 s), Flash 18 Hash verifiziert; Boot/Capture-Test
erfolgreich. Anschliessend meldet Nutzer auf die Build-28-Bildkorrektur-Frage
"Ja, sichtbar". Zu diesem Zeitpunkt war bereits Build 29 aufgespielt.
Damit ist die sichtbare Anzeige bestaetigt, die einzelne kausale Wirkung von
Zeilenpaaren gegenueber GPIO-Korrektur jedoch nicht getrennt nachgewiesen.
Vollstaendiger Build-29-Geraetetest `retro-ui-device-02` bestanden:
Load, dynamische Options/Editor, Farbwahl, MM IV e2e4/e7e5 und danach g1f3,
Board/Notation/Info, Quit, MM V als Weiss mit d2d4, erneuter Move/Abbruch.
Zwoelf saubere PNG-Scanlineaufnahmen und evidence.json gespeichert.
Diese Eingaben wurden per USB in denselben UI-Tap-Handler eingespeist;
physischer Kaltstart und Finger-Load-Test erneut beim Nutzer angefragt.
Nutzer bestaetigt anschliessend: "Bild und Touch funktionieren" nach USB
trennen/verbinden und Finger-Load. Build 29 ist damit der gesicherte sichtbare
Retro-Teststand unter backups/takeover-20261110/retro-ui-build29.
Das Verschlanken ist bis zum funktionierenden sichtbaren Display zurueckgestellt.

Build-26-Kandidat mit Firmware/ELF und Quellen separat gesichert unter
backups/takeover-20261110/retro-ui-build26. Es ist kein freigegebener physischer
Displaystand. Stabile nichtgrafische Core-v1-Sicherung bleibt unveraendert.

## Entscheidung: MAME behalten und gezielt spezialisieren

Empfehlung: vorerst keinen neuen Emulator schreiben. Der gemessene Hauptblock
ist der generische Speicherzugriff: etwa 4.884 MB zugeordnete Symbolgroessen
im gesicherten ELF, Adress-/Groessenalias-Duplikate entfernt. Dies ist eine
Symbolanalyse, keine Garantie fuer eine gleich grosse Flash-Ersparnis.
Es gibt bereits echte ROM-/UCI-Nachweise und reproduzierbare Vergleichstests.

Erster Kandidat ist deshalb ein explizites 65xx-Speicherprofil fuer native
8-Bit-Zugriffe, Little Endian, Adressschritt 0. Es behaelt Adressbreiten,
Mapping, Zeitsteuerung und breite logische Lese-/Schreibzugriffe bei, bindet
aber die unbenutzten nativen 16/32/64-Bit- und Big-Endian-Spezialisierungen
nicht mehr ein. Nicht passende Treiber muessen sichtbar abgelehnt werden.
Weitere CPU-Familien erhalten spaeter ihre benoetigten Busprofile; sie sind
damit noch nicht implementiert oder getestet.

`tools/prepare_65xx_memory.py` ist ab Build 30 integriert. CMake-Option
`MAME_S3_65XX_MEMORY` ist standardmaessig ON; OFF baut den vollen Allocator.
Build 30 erfolgreich in 96.06 s, Flash 19 Hash verifiziert. Die gemeldete
Firmwaregroesse sinkt von 9,776,816 auf 4,686,292 Bytes, also um 5,090,524 Bytes
(52.1%). Statisches RAM bleibt 68,644 Bytes; dies ist keine PSRAM-Einsparung.
Load-cycle-04 besteht sechs abwechselnde MM IV/MM V ROM-Starts mit echten
Antworten e7e5/c7c5, Ergebnis 0 und intakten Heaps bei --max-idle-loss 0.
Idle-PSRAM nach jedem Lauf exakt 8,311,432 Bytes; internes RAM alterniert
240919/240903 Bytes (letzter MM V 240907). Kein wachsender Verlust.
Das Profil protokolliert CPU-Adressenbreite 16 und generischen Dummy-Raum 32.
Der unverkleinerte stabile Kern bleibt unter backups/.../core-stable-v1;
die sichtbare, per Finger und Kaltstart bestaetigte UI unter retro-ui-build29.
Die Rueckkehr der gesamten Bauerino-Oberflaeche ist in 18_bauerino_return.md
beschrieben. Inzwischen ist Combined Build 09 / Flash 05 mit echtem MAME,
urspruenglichem Launcher, Virginia/Bots und Weboberflaeche erfolgreich getestet.
Der Nutzer bestaetigt Kaltstart, Animation, Finger-Touch und Telefon-Webzugriff.
Dieser gemeinsame Stand liegt gesichert unter `backups/.../combined-build09`.

Ein kompletter Ersatz muesste neben CPU-Befehlen auch Bus, Geraete, Interrupts,
Sensorbrett, Ausgaben und Zeitverhalten neu abbilden. MAME beschreibt diese
Aufteilung in seiner [Speicherarchitektur](https://docs.mamedev.org/techspecs/memory.html)
und den [Geraeteschnittstellen](https://docs.mamedev.org/techspecs/).
Ein separater kleiner Kern bleibt eine spaetere Option, falls die gemessenen
Ergebnisse der gezielten Spezialisierung fuer den ESP nicht ausreichen.
