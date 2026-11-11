#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
inspect_refs.py - Kommunikationskette und API-Oberflaeche der Referenzen.

Liefert:
  * Kompatibilitaetsmatrix: MAME-0.220-Schachsets  <->  chessengine-Interface-Datei
  * verwendete MAME-Lua-API-Symbole (Plugin-Kompatibilitaet gegen 0.220)
  * Inhalt von init.lua und eines Beispiel-Interfaces
  * Inhalt/Aufbau von MAME-4-PicoChess (AddMAME.sh, mame_emulation_new, armv7l-MAME-new)
  * gefundene MAME-Versionsangaben in beiden Referenzen
Ausgabe: docs/02_refs.md und docs/02_refs.json
"""
import io
import os
import re
import json
import collections
import datetime

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
REF = os.path.join(ROOT, "ref")
DOCS = os.path.join(ROOT, "docs")

PLUG = os.path.join(REF, "mame-chessengine", "chessengine")
PICO = os.path.join(REF, "MAME-4-PicoChess")

API_RE = re.compile(r'\b(manager|machine|emu|ui|screen|image|device|mem|ioport|'
                    r'tap|natkeyboard|render|video|sound|ram|input)\.[A-Za-z_][\w.]*')
VER_RE = re.compile(r'\b0\.2[0-9]{2}\b')


def read(p, limit=None):
    try:
        with io.open(p, "r", encoding="utf-8", errors="replace") as f:
            t = f.read()
        return t if limit is None else t[:limit]
    except Exception as e:
        return "ERR %r" % e


def main():
    out = {"generated_utc": datetime.datetime.now(datetime.timezone.utc)
           .strftime("%Y-%m-%dT%H:%M:%SZ")}

    # ---- 1) Plugin-Interfaces
    idir = os.path.join(PLUG, "interfaces")
    ifaces = sorted(f[:-4] for f in os.listdir(idir) if f.endswith(".lua")) if os.path.isdir(idir) else []
    out["plugin"] = {
        "path": os.path.relpath(PLUG, ROOT),
        "interface_count": len(ifaces),
        "interfaces": ifaces,
        "has_mm1": "mm1" in ifaces, "has_mm2": "mm2" in ifaces,
        "has_mm4": "mm4" in ifaces, "has_mm5": "mm5" in ifaces,
        "mephisto_like": [i for i in ifaces if re.match(
            r'^(mm|meph|academy|alm|berl|polgar|port|van|lond|lyon|mega|milano|montec|mondial|modena|nshort|mrisc|rebel)', i)],
        "files": sorted(os.listdir(PLUG)) if os.path.isdir(PLUG) else [],
    }

    # ---- 2) Lua-API-Nutzung
    syms = collections.Counter()
    srcs = ([os.path.join(PLUG, "init.lua")] +
            [os.path.join(idir, f) for f in os.listdir(idir)]) if os.path.isdir(idir) else []
    for s in srcs:
        for m in API_RE.findall(read(s)):
            syms[m] += 1
    top = collections.Counter()
    for k, v in syms.items():
        top[".".join(k.split(".")[:2])] += v
    out["api"] = {"top_level": dict(top.most_common(40)),
                  "detail_top": dict(syms.most_common(40))}

    out["init_lua"] = read(os.path.join(PLUG, "init.lua"))
    sample = "polgar" if "polgar" in ifaces else (ifaces[0] if ifaces else None)
    out["sample_interface_name"] = sample
    out["sample_interface"] = read(os.path.join(idir, sample + ".lua")) if sample else ""
    out["plugin_json"] = read(os.path.join(PLUG, "plugin.json"))

    # ---- 3) MAME-4-PicoChess
    pico = {"files": [], "versions": {}}
    if os.path.isdir(PICO):
        for dirpath, dirnames, filenames in os.walk(PICO):
            if ".git" in dirpath:
                continue
            rel = os.path.relpath(dirpath, PICO)
            pico["files"].append(rel + "/ (%d Dateien)" % len(filenames))
            if len(pico["files"]) > 60:
                break
        for name in ("README.md", "AddMAME.sh", "License"):
            p = os.path.join(PICO, name)
            if os.path.isfile(p):
                pico[name] = read(p, 6000)
        for name in ("mame_emulation_new", "armv7l-MAME-new"):
            p = os.path.join(PICO, name)
            if os.path.isdir(p):
                pico["listing_" + name] = sorted(os.listdir(p))[:40]
            elif os.path.isfile(p):
                pico["file_" + name] = read(p, 4000)
        for dirpath, _, filenames in os.walk(PICO):
            if ".git" in dirpath:
                continue
            for fn in filenames:
                if fn.lower().endswith((".sh", ".md", ".txt", ".ini", ".cfg", ".py")):
                    t = read(os.path.join(dirpath, fn))
                    vs = set(VER_RE.findall(t))
                    if vs:
                        pico["versions"][os.path.relpath(os.path.join(dirpath, fn), PICO)] = sorted(vs)
    out["picochess_4"] = pico

    with io.open(os.path.join(DOCS, "02_refs.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    L = ["# 02 - Referenzanalyse: Plugin, API, PicoChess-Integration", "",
         "Erzeugt: %s" % out["generated_utc"], "",
         "## chessengine-Plugin (sronco, Repo-Tip)", "",
         "Pfad: `%s`" % out["plugin"]["path"],
         "Interface-Dateien: **%d**" % out["plugin"]["interface_count"], "",
         "MM-IV/MM-V-Interface vorhanden? mm1=%s mm2=%s **mm4=%s mm5=%s**"
         % (out["plugin"]["has_mm1"], out["plugin"]["has_mm2"],
            out["plugin"]["has_mm4"], out["plugin"]["has_mm5"]), "",
         "Mephisto-artige Interfaces (%d):" % len(out["plugin"]["mephisto_like"]), "",
         "```", "\n".join(out["plugin"]["mephisto_like"]), "```", "",
         "## Verwendete MAME-Lua-API (Top-Level, Haeufigkeit)", "",
         "| Symbol | Treffer |", "|---|---:|"]
    for k, v in out["api"]["top_level"].items():
        L.append("| `%s` | %d |" % (k, v))
    L += ["", "## `plugin.json`", "", "```json", out["plugin_json"], "```",
          "", "## `init.lua`", "", "```lua", out["init_lua"], "```", "",
          "## Beispiel-Interface `%s.lua`" % out["sample_interface_name"], "",
          "```lua", out["sample_interface"], "```", "",
          "## MAME-4-PicoChess (ScallyBag)", "",
          "Verzeichnisse (Auszug):", "", "```", "\n".join(pico["files"][:40]), "```", ""]
    if pico.get("versions"):
        L += ["Gefundene Versionsangaben:", "", "| Datei | 0.2xx |", "|---|---|"]
        for k, v in sorted(pico["versions"].items()):
            L.append("| `%s` | %s |" % (k, ", ".join(v)))
    else:
        L.append("_keine 0.2xx-Versionsangaben in Textdateien gefunden_")
    for key, label in (("listing_mame_emulation_new", "mame_emulation_new/"),
                       ("listing_armv7l-MAME-new", "armv7l-MAME-new/")):
        if key in pico:
            L += ["", "### Inhalt `%s`" % label, "", "```", "\n".join(pico[key]), "```"]
    for name in ("README.md", "AddMAME.sh"):
        if name in pico:
            L += ["", "### `%s` (Auszug)" % name, "", "```", pico[name][:4500], "```"]
    with io.open(os.path.join(DOCS, "02_refs.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("interfaces=%d mm4=%s mm5=%s api_symbols=%d pico_versions=%d"
          % (len(ifaces), out["plugin"]["has_mm4"], out["plugin"]["has_mm5"],
             len(syms), len(pico.get("versions", {}))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
