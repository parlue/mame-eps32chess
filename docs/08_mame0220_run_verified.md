# 08 - MAME 0.220 läuft: Laufnachweis und Ablage der Teilpakete

Erzeugt: 2026-11-09. Dieses Dokument belegt **ausgeführte** Befehle und ihre
Ausgaben. Es ist kein Plan.

## 1. Herkunft und Integrität

| Punkt | Wert |
|---|---|
| Asset | `mame0220b_64bit.exe` (offizielles GitHub-Release-Asset zum Tag `mame0220`) |
| Größe | 77.877.048 Bytes |
| SHA256 erwartet (Release-`SHA256SUMS`) | `73738adb1e66f8f3fb505b54a52dc1c90b1529698420ada2b5a480ea43f404b4` |
| SHA256 gemessen | identisch → **OK** |
| Ablage | `mame-s3/ref/mame0220-win/` |

## 2. Entpackt und gestartet (verifiziert)

```powershell
$bin = (Resolve-Path 'mame-s3\ref\mame0220-win\bin').Path
& (Join-Path $bin 'mame64.exe') -version
```
Ausgabe:
```
0.220 (mame0220)
```

```powershell
& (Join-Path $bin 'mame64.exe') -listroms mm4
```
Ausgabe:
```
ROMs required for driver "mm4".
Name                                   Size Checksum
710                                   32768 CRC(f68a4124) SHA1(d1d03a9aacc291d5cb720d2ee2a209eeba13a36c)
```

```powershell
& (Join-Path $bin 'mame64.exe') -listroms mm5
```
Ausgabe:
```
ROMs required for driver "mm5".
Name                                   Size Checksum
mephisto5.rom                         32768 CRC(89c3d9d2) SHA1(77cd6f8eeb03c713249db140d2541e3264328048)
```

**Bedeutung:** Die laufende 0.220-Binärdatei bestätigt unabhängig die aus dem
Treibercode (`src/mame/drivers/mephisto_mm2.cpp`) extrahierten ROM-Metadaten
(§`docs/03_rom_manifest.md`). Zwei getrennte Quellen, identisches Ergebnis:
Datei `710`, 32768 B, CRC32 `f68a4124`.

Damit ist Nachweis **Stufe 1 (Programm existiert und läuft)** für die Host-Seite
erbracht. Ausdrücklich **nicht** erbracht: ROM gestartet, Plugin geladen, Engine
im Spiel, S3-Firmware.

## 3. Lieferumfang der entpackten Fassung

`mame-s3/ref/mame0220-win/bin/`:

| Objekt | Inhalt |
|---|---|
| `mame64.exe` | 266.252.288 Bytes |
| weitere Tools | `castool`, `chdman`, `floptool`, `imgtool`, `jedutil`, `ldresample`, `ldverify`, `nltool`, `nlwav`, `romcmp`, `unidasm` |
| Ordner | `artwork/`, `bgfx/`, `ctrlr/`, `docs/`, `hash/`, `hlsl/`, `ini/`, `language/`, `plugins/`, `roms/`, `samples/` |
| Dateien | `COPYING`, `mame64.sym`, `uismall.bdf`, `whatsnew.txt` |

`plugins/` enthält: `autofire, cheat, cheatfind, console, data, discord, dummy,
gdbstub, hiscore, json, layout, portname, timer, boot.lua, README.md`
→ **kein `chessengine`**. Das Plugin ist out-of-tree und muss hier hinein.

## 4. Wohin die noch fehlenden Teile gehören

| Teil | Zielort in dieser Ablage |
|---|---|
| chessengine-Plugin (aus CB-Emu, 0.220-Fassung) | `ref/mame0220-win/bin/plugins/chessengine/` (mit `init.lua`, `plugin.json`, `interfaces/`) |
| ROM-Satz MM IV | `ref/mame0220-win/bin/roms/mm4/` mit Datei `710` (32768 B, CRC32 `f68a4124`) — oder als `roms/mm4.zip` mit demselben Member |
| ROM-Satz MM V (optional) | `ref/mame0220-win/bin/roms/mm5/` mit `mephisto5.rom` (CRC32 `89c3d9d2`) |

Prüfung erfolgt automatisch mit `python mame-s3\tools\inventory.py`
(liest CRC32 aus dem ZIP-Verzeichnis; keine ROM-Inhalte werden ausgegeben).

## 5. Referenz-Kommandozeile (aus dem PicoChess/Plugin-Umfeld übernommen)

```
mame64.exe -skip_gameinfo -video none -plugin chessengine mm4
```
Anmerkung: `-video none` ist die Vorgehensweise aus dem Linux-/SDL-Umfeld. Ob
diese Windows-Fassung das Video-Modul `none` kennt, ist **noch nicht geprüft**
und wird erst mit vorhandenem Plugin/ROM getestet. Bis dahin wird nichts als
„headless läuft" behauptet.

## 6. Grenzen dieses Nachweises

* Kein ROM vorhanden → kein Start der Emulation.
* Kein `chessengine`-Plugin vorhanden → keine Engine-Schnittstelle.
* Keine ESP32-S3-Hardware → kein Hardwaretest.
* Der Hostlauf ist ein **Referenzmaßstab**, kein Auslieferprodukt.
