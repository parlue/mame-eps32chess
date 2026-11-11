#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
find_drivers.py - Treiberkandidaten fuer Schachcomputer in MAME 0.220.

Laedt src/mame/mame.lst (Tag mame0220) und filtert die Set-/Treibernamen.
Schreibt docs/01_drivers.md und legt mame.lst als Referenz in docs/ ab.
mame.lst enthaelt nur Set-Namen/Treiberpfade, keine ROM-Daten.
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
URL = "https://raw.githubusercontent.com/mamedev/mame/mame0220/src/mame/mame.lst"

# engere Liste bekannter Schachcomputer-Produktnamen im MAME-Namensraum
PRODUCT = re.compile(
    r'\b(mm[0-9]|mm[ivx]+|mephisto|excalibur|novag|saitek|fidelity|conchess|'
    r'mchess|chesson|chessm|chessmate|compuchess|cchess|modular|milton|'
    r'mondial|romag|kasparov|tasc|renaissance|spracklen|sargon|chafitz|'
    r'applied.?concepts|gmk|igaco|destiny|expert|priviet|schach)\b', re.I)


def main():
    os.makedirs(DOCS, exist_ok=True)
    r = requests.get(URL, headers=UA, timeout=60)
    r.raise_for_status()
    txt = r.text
    with io.open(os.path.join(DOCS, "mame0220_mame.lst.txt"), "w",
                 encoding="utf-8", newline="\n") as f:
        f.write(txt)
    lines = txt.splitlines()

    # Formatprobe
    head = lines[:40]

    hits = []
    cur_src = "(vor erster @source-Zeile)"
    for i, ln in enumerate(lines):
        s = ln.strip()
        if s.startswith("@source:"):
            cur_src = s
            continue
        if PRODUCT.search(ln) and not s.startswith("//"):
            hits.append({"line_no": i + 1, "line": s, "source": cur_src})

    by_src = {}
    for h in hits:
        by_src.setdefault(h["source"], []).append("%s   (mame.lst Zeile %d)" % (h["line"], h["line_no"]))

    out = {"url": URL, "total_lines": len(lines), "format_head": head,
           "hit_count": len(hits), "by_source": by_src, "hits": hits}
    with io.open(os.path.join(DOCS, "01_drivers.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    L = ["# 01 - Schachcomputer-Treiber in MAME 0.220", "",
         "Quelle: `%s`" % URL,
         "Zeilen: %d | Treffer: %d" % (len(lines), len(hits)),
         "Erzeugt: %s" % datetime.datetime.now(datetime.timezone.utc)
         .strftime("%Y-%m-%dT%H:%M:%SZ"), "",
         "## Treffer gruppiert nach Treiberdatei", ""]
    for src, sets in sorted(by_src.items()):
        L += ["### `%s`" % src, "", "```", "\n".join(sorted(sets)), "```", ""]
    with io.open(os.path.join(DOCS, "01_drivers.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("hits=%d sources=%d" % (len(hits), len(by_src)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
