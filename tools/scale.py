#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
scale.py - Groesse/Umfang des MAME-0.220-Quellstands (Messwerte, keine Schaetzung).

Ein API-Aufruf: git/trees/mame0220?recursive=1 liefert alle Pfade + Blob-Groessen.
Ausgabe: docs/04_scale.md + docs/04_scale.json
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

INTEREST = ["src/emu", "src/devices", "src/mame", "src/frontend", "src/lib",
            "src/osd", "src/tools", "3rdparty", "plugins", "scripts",
            "src/mame/drivers", "src/mame/machine", "src/mame/video", "src/mame/audio"]


def main():
    r = requests.get(URL, headers=UA, timeout=120)
    r.raise_for_status()
    data = r.json()
    tree = data.get("tree", [])
    out = {"url": URL, "tree_truncated": data.get("truncated", None),
           "entries": len(tree),
           "generated_utc": datetime.datetime.now(datetime.timezone.utc)
           .strftime("%Y-%m-%dT%H:%M:%SZ")}

    blobs = [t for t in tree if t.get("type") == "blob"]
    out["blob_count"] = len(blobs)
    out["total_bytes"] = sum(b.get("size", 0) for b in blobs)

    by_ext = collections.Counter()
    bytes_by_ext = collections.Counter()
    for b in blobs:
        ext = os.path.splitext(b["path"])[1].lower() or "(none)"
        by_ext[ext] += 1
        bytes_by_ext[ext] += b.get("size", 0)
    out["files_by_ext"] = dict(by_ext.most_common(15))
    out["bytes_by_ext"] = dict(bytes_by_ext.most_common(15))

    per_dir = {}
    for d in INTEREST:
        pre = d + "/"
        sel = [b for b in blobs if b["path"].startswith(pre)]
        cpp = [b for b in sel if b["path"].lower().endswith((".cpp", ".h", ".hpp", ".c"))]
        per_dir[d] = {
            "files": len(sel), "code_files": len(cpp),
            "bytes": sum(b.get("size", 0) for b in sel),
            "code_bytes": sum(b.get("size", 0) for b in cpp),
        }
    out["per_dir"] = per_dir
    out["thirdparty_top"] = dict(collections.Counter(
        b["path"].split("/")[1] for b in blobs if b["path"].startswith("3rdparty/") and len(b["path"].split("/")) > 1).most_common(30))

    with io.open(os.path.join(DOCS, "04_scale.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    mb = lambda n: n / 1048576.0
    L = ["# 04 - Umfang des MAME-0.220-Quellstands (Messwerte)", "",
         "Quelle: `%s`" % URL,
         "Erzeugt: %s" % out["generated_utc"],
         "Tree-Eintraege: %d (truncated=%s), davon Dateien (blobs): **%d**"
         % (out["entries"], out["tree_truncated"], out["blob_count"]),
         "Gesamtgroesse aller Dateien: **%.1f MB**" % mb(out["total_bytes"]), "",
         "## Dateien je Endung (Top 15)", "", "| Endung | Dateien | MB |", "|---|---:|---:|"]
    for ext, n in out["files_by_ext"].items():
        L.append("| `%s` | %d | %.1f |" % (ext, n, mb(bytes_by_ext[ext])))
    L += ["", "## Verzeichnisse, die fuer eine Portierung relevant sind", "",
          "| Verzeichnis | Dateien | C/C++-Dateien | MB | C/C++ MB |",
          "|---|---:|---:|---:|---:|"]
    for d, v in per_dir.items():
        L.append("| `%s` | %d | %d | %.1f | %.1f |"
                 % (d, v["files"], v["code_files"], mb(v["bytes"]), mb(v["code_bytes"])))
    L += ["", "## `3rdparty/` (Top 30 Unterverzeichnisse)", "", "| Unterverzeichnis | Dateien |",
          "|---|---:|"]
    for k, v in out["thirdparty_top"].items():
        L.append("| `%s` | %d |" % (k, v))
    with io.open(os.path.join(DOCS, "04_scale.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("blobs=%d total=%.1f MB src/emu=%d files src/mame=%d files"
          % (out["blob_count"], mb(out["total_bytes"]),
             per_dir["src/emu"]["files"], per_dir["src/mame"]["files"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
