# Herkunft und Lizenzen

Dieses Repository enthält den ESP32-S3-Port und die kombinierte Bauerino-
Anwendung. Copyright- und Lizenzangaben in den einzelnen Dateien bleiben
maßgeblich. `LICENSE` enthält GPL-3.0 für die Bauerino-/Virgin-Anwendung;
dies ist keine Neulizenzierung fremder Komponenten.

- MAME/MESS 0.220: MAME-Team und jeweilige Dateiautoren. Der unveränderte
  Quellbaum wird separat bezogen; seine Lizenzübersicht liegt hier unter
  `licenses/MAME-COPYING`. Die Portdateien und generierten Overlays bewahren
  die ursprünglichen Dateilizenzen. Quelle: https://github.com/mamedev/mame/tree/mame0220
- Virginia/Virgin und Bauerino: siehe `bauerino/LICENSE` und
  `bauerino/THIRD_PARTY.md`, einschließlich Grafik- und Brettbibliotheken.
- MessChess/chessengine: Sandro Ronco und Franz Huber, Version 2024.1.1;
  BSD-3-Clause laut Plugin-Metadaten. Siehe
  `integration/chessengine/NOTICE.md` und `licenses/MessChess-BSD-3-Clause.txt`.
  Die Anpassung bewahrt die Autorenangaben im Lua-Payload.
- Lua 5.3.4: Lua.org / PUC-Rio, MIT; über den unveränderten MAME-Quellbaum
  bezogen. Lizenztext in dessen `3rdparty/lua/src/lua.h`.
- Arduino, ESP-IDF, NimBLE, GFX und SensorLib werden über die dokumentierten
  Build-Abhängigkeiten unter ihren jeweiligen Lizenzen bezogen.

Schachcomputer-ROMs, ROM-Dateisysteme und komplette Flash-Sicherungen sind
nicht Bestandteil dieses Repositorys. Der lokale ROM-Importer benötigt vom
Anwender separat bereitgestellte Dateien.
