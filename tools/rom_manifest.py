#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rom_manifest.py - ROM-Metadaten des ersten Zielmodells aus MAME 0.220.

Laedt src/mame/drivers/mephisto_mm2.cpp (Tag mame0220) und extrahiert fuer die
Sets mm2, mm4, mm4tk, mm5, bup, rebel5:
  * ROM_REGION- und ROM_LOAD*-Angaben (Dateiname, Offset, Groesse, CRC, SHA1)
  * CPU-Typ und Taktangaben aus dem Treiber
Es werden KEINE ROM-Inhalte geladen oder gespeichert - nur Metadaten.
Ausgabe: docs/03_rom_manifest.md + docs/03_rom_manifest.json
"""
import io
import os
import re
import json
import datetime
import requests

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DOCS = os.path.join(ROOT, "docs")
UA = {"User-Agent": "mame-s3-analysis/0.1 (+local research)"}
URL = "https://raw.githubusercontent.com/mamedev/mame/mame0220/src/mame/drivers/mephisto_mm2.cpp"

SETS = ["mm2", "mm2a", "mm2b", "mm2c", "mm2d", "mm4", "mm4a", "mm4b", "mm4tk",
        "mm5", "mm5a", "bup", "bupa", "rebel5", "rebel5a"]

CLK_RE = re.compile(r'XTAL\(([\d\']+)\)')
REGION_RE = re.compile(r'ROM_REGION\(\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*"([^"]+)"\s*,\s*([^)]*)\)')
CRC_RE = re.compile(r'CRC\(([0-9a-fA-F]+)\)')
SHA1_RE = re.compile(r'SHA1\(([0-9a-fA-F]+)\)')
FILE_RE = re.compile(r'"([^"]+)"')

REGION_MACROS = ["ROM_REGION", "ROM_REGION16_BE", "ROM_REGION32_BE",
                 "ROM_REGION32_LE", "ROM_REGION64_LE"]
LOAD_MACROS = ["ROM_LOAD", "ROMX_LOAD", "ROM_LOAD16_BYTE", "ROM_LOAD16_WORD",
               "ROM_LOAD32_BYTE", "ROM_LOAD32_WORD", "ROM_LOAD_OPTIONAL",
               "ROMX_LOAD_OPTIONAL", "ROM_CONTINUE", "ROM_RELOAD"]
CPU_MACROS = ["M6502", "M65C02", "Z80", "M68000", "M6809", "I8085", "M6802", "M6803"]


def fetch(url):
    r = requests.get(url, headers=UA, timeout=60)
    r.raise_for_status()
    return r.text


def balanced(text, start):
    depth, i = 0, start
    while i < len(text):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
        i += 1
    return text[start + 1:]


def calls(text, names):
    out = []
    pat = re.compile(r'\b(' + "|".join(names) + r')\s*\(')
    for m in pat.finditer(text):
        out.append((m.group(1), balanced(text, m.end() - 1), m.start()))
    return out


def block_for(text, setname):
    m = re.search(r'ROM_START\s*\(\s*' + re.escape(setname) + r'\s*\)', text)
    if not m:
        return None
    end = text.find("ROM_END", m.end())
    return text[m.end():end if end > 0 else len(text)]


def parse_block(blk):
    regions, roms = [], []
    for name, args, _ in calls(blk, REGION_MACROS):
        rm = REGION_RE.search(name + "(" + args + ")")
        if rm:
            regions.append({"size": rm.group(1), "tag": rm.group(2),
                            "flags": rm.group(3).strip()})
    for name, args, _ in calls(blk, LOAD_MACROS):
        files = FILE_RE.findall(args)
        head = args.split(",")
        e = {"macro": name, "file": files[0] if files else None,
             "offset": None, "size": None}
        if name not in ("ROM_CONTINUE", "ROM_RELOAD") and len(head) >= 3:
            e["offset"] = head[1].strip()
            e["size"] = head[2].strip()
        crc, sha = CRC_RE.search(args), SHA1_RE.search(args)
        e["crc"] = crc.group(1) if crc else None
        e["sha1"] = sha.group(1) if sha else None
        roms.append(e)
    return regions, roms


def main():
    text = fetch(URL)
    out = {"url": URL, "sets": {}, "generated_utc":
           datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}
    for s in SETS:
        blk = block_for(text, s)
        if blk is None:
            out["sets"][s] = {"found": False}
            continue
        regions, roms = parse_block(blk)
        out["sets"][s] = {"found": True, "regions": regions, "roms": roms}
    out["cpu_hits"] = [{"cpu": n, "args": a[:140].replace("\n", " ")}
                       for n, a, _ in calls(text, CPU_MACROS)]
    out["clock_hits"] = sorted(set(CLK_RE.findall(text)))

    with io.open(os.path.join(DOCS, "03_rom_manifest.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    L = ["# 03 - Modellwahl und ROM-Manifest (MAME 0.220)", "",
         "Treiberquelle: `%s`" % URL,
         "Erzeugt: %s" % out["generated_utc"], "",
         "**Nur ROM-Metadaten (Dateiname, Groesse, CRC, SHA1). Es werden keine "
         "ROM-Inhalte geladen, gespeichert oder weitergegeben.**", ""]
    for s in SETS:
        e = out["sets"][s]
        L += ["## Set `%s`" % s, ""]
        if not e.get("found"):
            L += ["_kein ROM_START-Block gefunden_", ""]
            continue
        L += ["ROM-Regionen:", "", "| Groesse | Tag | Flags |", "|---|---|---|"]
        for r in e["regions"]:
            L.append("| `%s` | `%s` | `%s` |" % (r["size"], r["tag"], r["flags"]))
        L += ["", "ROM-Eintraege:", "",
              "| Makro | Datei | Offset | Groesse | CRC | SHA1 |",
              "|---|---|---|---|---|---|"]
        for r in e["roms"]:
            L.append("| `%s` | `%s` | %s | %s | `%s` | `%s` |" % (
                r["macro"], r["file"], r.get("offset") or "—", r.get("size") or "—",
                r.get("crc") or "—", r.get("sha1") or "—"))
        L.append("")
    L += ["## CPU-/Taktangaben im Treiber", "",
          "Gefundene Taktangaben: %s"
          % (", ".join("`XTAL(%s)`" % c for c in out["clock_hits"]) or "(keine)"),
          "", "CPU-Makros (%d):" % len(out["cpu_hits"]), ""]
    for h in out["cpu_hits"][:15]:
        L.append("* `%s(%s)`" % (h["cpu"], h["args"][:100]))
    with io.open(os.path.join(DOCS, "03_rom_manifest.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("sets=%d mm4=%s mm5=%s roms_mm4=%d" % (
        len(SETS), out["sets"]["mm4"].get("found"), out["sets"]["mm5"].get("found"),
        len(out["sets"]["mm4"].get("roms", []))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
