#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
port_scope.py - Messwerte fuer den Portierungsumfang von MAME 0.220.

Ein API-Aufruf (git/trees/mame0220?recursive=1). Ausgewertet werden
Verzeichnisgroessen fuer
  * was ein 8-Bit-Schachcomputer-Treiber (mm4: 65C02) minimal braucht
  * was mit hoher Wahrscheinlichkeit entfallen kann (Grafik/Audio/Artwork)
  * Laufzeitabhaengigkeiten (Lua, sol2, softfloat, zlib, exfat/FAT)

Ergebnis: docs/06_port_scope.md + docs/06_port_scope.json
Alle Zahlen sind gemessene Dateigroessen aus dem Git-Baum, keine Schaetzung.
"""
import io
import os
import json
import collections
import datetime
import requests

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DOCS = os.path.join(ROOT, "docs")
UA = {"User-Agent": "mame-s3-analysis/0.1 (+local research)"}
URL = "https://api.github.com/repos/mamedev/mame/git/trees/mame0220?recursive=1"

# Verzeichnisse, die ein 8-Bit-Treiber realistisch braucht
NEEDED = [
    "src/emu",                       # Geraete-/Systemkern
    "src/devices/cpu/m6502",          # CPU des mm4 (65C02)
    "src/devices/cpu",                # weitere CPUs nur als Ueberblick
    "src/devices/machine",            # generische Geraete (Timer, Speicher, I/O)
    "src/devices/sound",              # Beeper/Sprachausgabe der Geraete
    "src/devices/video",              # LCD-Ansteuerung
    "src/devices/bus",                # Anschluss/Aufsatzmodule
    "src/devices/imagedev",           # Kassette/Modul/Disk
    "src/lib/util",                   # Kern-Utilities (attotime, hash, ...)
    "src/lib",                        # gesamte lib
    "src/osd",                        # Plattformschicht (muss ersetzt werden)
    "plugins",                        # Lua-Plugins (Laufzeit)
    "3rdparty/lua", "3rdparty/sol2", "3rdparty/luasocket", "3rdparty/luafilesystem",
    "3rdparty/softfloat3", "3rdparty/softfloat",
    "3rdparty/zlib", "3rdparty/expat", "3rdparty/pugixml", "3rdparty/rapidjson",
    "3rdparty/utf8proc", "3rdparty/linenoise", "3rdparty/lsqlite3", "3rdparty/lua-zlib",
    "3rdparty/libflac", "3rdparty/portaudio", "3rdparty/portmidi",
    "3rdparty/bgfx", "3rdparty/bimg", "3rdparty/bx", "3rdparty/glm", "3rdparty/SDL2",
    "3rdparty/asio", "3rdparty/lzma", "3rdparty/nanosvg",
]


def main():
    r = requests.get(URL, headers=UA, timeout=120)
    r.raise_for_status()
    data = r.json()
    blobs = [t for t in data.get("tree", []) if t.get("type") == "blob"]

    out = {"url": URL, "generated_utc": datetime.datetime.now(datetime.timezone.utc)
           .strftime("%Y-%m-%dT%H:%M:%SZ"), "dirs": {}}
    for d in NEEDED:
        pre = d + "/"
        sel = [b for b in blobs if b["path"].startswith(pre)]
        code = [b for b in sel if b["path"].lower().endswith((".cpp", ".h", ".hpp", ".c", ".cc"))]
        out["dirs"][d] = {"files": len(sel), "code_files": len(code),
                          "bytes": sum(b.get("size", 0) for b in sel),
                          "code_bytes": sum(b.get("size", 0) for b in code)}

    # Artwork/Layout und Treiber ausserhalb des Ziels
    lay = [b for b in blobs if b["path"].lower().endswith(".lay")]
    drv = [b for b in blobs if b["path"].startswith("src/mame/drivers/")]
    out["droppable"] = {
        "layout_lay_files": len(lay), "layout_lay_bytes": sum(b.get("size", 0) for b in lay),
        "driver_files_total": len(drv),
        "driver_bytes_total": sum(b.get("size", 0) for b in drv),
    }
    out["m6502_files"] = sorted(b["path"] for b in blobs
                                if b["path"].startswith("src/devices/cpu/m6502/"))

    with io.open(os.path.join(DOCS, "06_port_scope.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    mb = lambda n: n / 1048576.0
    L = ["# 06 - Portierungsumfang: was ein 8-Bit-Treiber braucht", "",
         "Quelle: `%s` (gemessene Blob-Groessen aus dem Git-Baum)" % URL,
         "Erzeugt: %s" % out["generated_utc"], "",
         "## Verzeichnisse", "",
         "| Verzeichnis | Dateien | C/C++ | MB | C/C++ MB | Einschaetzung |",
         "|---|---:|---:|---:|---:|---|"]
    DROP = ("bgfx", "bimg", "bx", "glm", "SDL2", "asio", "portaudio", "portmidi",
            "libflac", "lzma", "nanosvg")
    for d, v in out["dirs"].items():
        est = "**entfaellt voraussichtlich**" if any(k in d for k in DROP) else \
              ("muss ersetzt werden" if d == "src/osd" else "vorgesehen")
        L.append("| `%s` | %d | %d | %.1f | %.1f | %s |"
                 % (d, v["files"], v["code_files"], mb(v["bytes"]), mb(v["code_bytes"]), est))
    L += ["", "## Entfallen kann (Artwork/Treiber ausserhalb des Ziels)", "",
          "* Layout-/Artwork-Dateien (`.lay`): **%d Dateien, %.1f MB**"
          % (out["droppable"]["layout_lay_files"], mb(out["droppable"]["layout_lay_bytes"])),
          "* Treiberdateien gesamt: %d Dateien, %.1f MB — gebraucht wird davon **eine**"
          % (out["droppable"]["driver_files_total"], mb(out["droppable"]["driver_bytes_total"])),
          "", "## `src/devices/cpu/m6502/` (CPU des mm4)", "",
          "```", "\n".join(out["m6502_files"]) or "(keine)", "```", "",
          "## Einordnung", "",
          "Die Zahlen belegen: der *Treiber* ist winzig, der *Rahmen* ist es nicht.",
          "Ein Herauslösen (CPU + Gerätesatz + Lua-Schnittstelle) ist damit eine",
          "berechenbare Option, aber ausdruecklich eine **Abweichung** vom vollen",
          "Portierungsansatz und nicht als gleichwertig darzustellen."]
    with io.open(os.path.join(DOCS, "06_port_scope.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("m6502 files=%d  src/emu=%.1f MB  drivers=%.1f MB  lay=%.1f MB"
          % (len(out["m6502_files"]), mb(out["dirs"]["src/emu"]["bytes"]),
             mb(out["droppable"]["driver_bytes_total"]),
             mb(out["droppable"]["layout_lay_bytes"])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
