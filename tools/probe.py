#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
probe.py - Bestandsaufnahme fuer den MAME/ESP32-S3-Auftrag.

Schreibt mame-s3/docs/00_sources.json und 00_sources.md:
  * geklonte Referenz-Repos + Commit-IDs + Top-Level-Inhalt
  * MAME 0.220 (Tag mame0220): Verzeichnisstruktur und Treiberkandidaten
  * ob Upstream-0.220 ein chessengine-Plugin enthaelt
Nur lesende Netzwerkzugriffe (GitHub contents API, rate-limitiert 60/h).
"""
import io
import os
import json
import subprocess
import datetime
import requests

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)              # mame-s3
REF = os.path.join(ROOT, "ref")
DOCS = os.path.join(ROOT, "docs")
UA = {"User-Agent": "mame-s3-analysis/0.1 (+local research)"}
TAG = "mame0220"
API = "https://api.github.com/repos/mamedev/mame/contents/%s?ref=" + TAG

# Treiber-Namensmuster fuer Schachcomputer
import re
PAT = re.compile(r'(mephisto|fidelity|novag|chess|saitek|conchess|tasc|mchess|cchess|rebel|radioshack|meg|sensor|milton|excalibur)', re.I)


def sh(*args):
    try:
        return subprocess.run(args, capture_output=True, text=True, timeout=60).stdout.strip()
    except Exception as e:
        return "ERR %r" % e


def api_listing(path):
    try:
        r = requests.get(API % path, headers=UA, timeout=30)
        if r.status_code != 200:
            return {"error": r.status_code, "body": r.text[:180]}
        return sorted(x["name"] for x in r.json())
    except Exception as e:
        return {"error": repr(e)}


def main():
    os.makedirs(DOCS, exist_ok=True)
    out = {
        "generated_utc": datetime.datetime.now(datetime.timezone.utc)
        .strftime("%Y-%m-%dT%H:%M:%SZ"),
        "host": {"platform": os.name, "python": subprocess.run(
            ["python", "--version"], capture_output=True, text=True).stdout.strip()},
        "repos": {},
        "mame0220": {},
    }

    # --- 1) geklonte Referenzen
    for name in sorted(os.listdir(REF)) if os.path.isdir(REF) else []:
        p = os.path.join(REF, name)
        if not os.path.isdir(p):
            continue
        top = sorted(os.listdir(p))[:40]
        out["repos"][name] = {
            "head": sh("git", "-C", p, "rev-parse", "HEAD"),
            "date": sh("git", "-C", p, "log", "-1", "--format=%cI"),
            "subject": sh("git", "-C", p, "log", "-1", "--format=%s"),
            "top_level": top,
            "file_count": sum(len(f) for _, _, f in os.walk(p)),
        }

    # --- 2) MAME 0.220 Struktur
    out["mame0220"]["src_mame_entries"] = api_listing("src/mame")
    for cand in ("src/mame/drivers", "src/mame/chess", "src/mame/misc"):
        got = api_listing(cand)
        if isinstance(got, list):
            out["mame0220"]["dir_" + cand.replace("/", "_")] = {
                "total": len(got), "chess_like": [n for n in got if PAT.search(n)]}
        else:
            out["mame0220"]["dir_" + cand.replace("/", "_")] = got
    out["mame0220"]["plugins"] = api_listing("plugins")

    with io.open(os.path.join(DOCS, "00_sources.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    L = ["# 00 - Quellen- und Versionsbestand", "",
         "Erzeugt: %s" % out["generated_utc"], "",
         "## Geklonte Referenzen (`mame-s3/ref/`)", "",
         "| Repo | Commit | Datum | Dateien | Thema |", "|---|---|---|---:|---|"]
    for n, r in out["repos"].items():
        L.append("| %s | `%s` | %s | %d | %s |" % (n, r["head"][:10], r["date"], r["file_count"], r["subject"]))
    L += ["", "## MAME 0.220 (`mame0220`)", ""]
    sm = out["mame0220"]["src_mame_entries"]
    if isinstance(sm, list):
        dirs = [x for x in sm if "." not in x]
        L.append("`src/mame` Unterordner (%d): %s" % (len(dirs), ", ".join(dirs[:30])))
    else:
        L.append("`src/mame` Abfrage fehlgeschlagen: %s" % sm)
    for k, v in out["mame0220"].items():
        if k.startswith("dir_") and isinstance(v, dict) and "chess_like" in v:
            L += ["", "`%s` -> %d Dateien, Trefferkandidaten (%d):" % (k[4:].replace("_", "/"), v["total"], len(v["chess_like"])),
                  "", "```", "\n".join(v["chess_like"]) or "(keine)", "```"]
        elif k.startswith("dir_") and isinstance(v, dict):
            L += ["", "`%s` -> Fehler %s" % (k[4:].replace("_", "/"), v)]
    pl = out["mame0220"]["plugins"]
    L += ["", "## `plugins/` in 0.220", "", "```",
          ("\n".join(pl) if isinstance(pl, list) else json.dumps(pl)), "```", ""]
    with io.open(os.path.join(DOCS, "00_sources.md"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("wrote docs/00_sources.json + .md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
