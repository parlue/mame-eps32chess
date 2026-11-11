# Herkunft
- M5Chess / Virgin, Dirk D. Sommerfeld, GPL-3.0.
  Vollständiger Lizenztext: LICENSE.
- Engine: eingefrorener M5Chess-Stand vom 03.11.2026.
  Originalquellen und SHA256: docs/virgin-import.json.
- Brettprotokoll und Game-Modell: ursprünglicher lokaler Import mit
  Quellen-Hashes in docs/m5chess-import.json. Dessen Regeln-Dateien wurden
  inzwischen durch den konsistenten eingefrorenen Engine-Stand ersetzt.
- Anpassungen am Enginecode: Identifikation ESPMAME/ESP32-S3, PSRAM-Allocator
  für Hash, Embedded-Hashgröße bei Sitzungsneustart und passender release-Typ.
  Details: docs/virgin.md.
- Game-Modell: kein automatischer Neustart mit Weiß beim Aufbau der Grundstellung.
- NimBLE-Arduino 2.5.1, h2zero, Apache-2.0; über PlatformIO geladen.
  https://github.com/h2zero/NimBLE-Arduino
- Arduino ESP32 / ESP-IDF und PlatformIO unter ihren jeweiligen Lizenzen.
- UCI-Sitzung, Zeilenkanal, Websteuerung und Adapter wurden für espmame erstellt.
- MAME/MESS, MessChess, Lua-Interfaces und ROMs sind noch nicht enthalten.

Bauerino display port:
- Queen moods and robot/glove: existing local M5Chess / BeatVirgin artwork,
  Dirk D. Sommerfeld. Original source images included in assets/bauerino.
- Merida pieces: Armando Hernandez Marroquin, GPLv2 or later; distributed
  under GPLv3. Original WebP artwork and conversion script included.
  See assets/bauerino/PIECES-LICENSE.txt.
- GFX Library for Arduino 1.6.4, Moon On Our Nation, BSD license.
  https://github.com/moononournation/Arduino_GFX
- SensorLib 0.3.3, lewis he, MIT license.
  https://github.com/lewisxhe/SensorLib
- The launcher icons for Retro, Setup and Analyse are project-generated artwork.
