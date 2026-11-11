#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
get_0220.py - offizielles MAME-0.220-Windows-Binary von GitHub holen und pruefen.

Quelle: Release-Assets zum Tag mame0220 (github.com/mamedev/mame/releases).
Prueft die SHA256 aus der mitgelieferten SHA256SUMS-Datei.
Ausgabe: mame-s3/ref/mame0220-win/ + docs/07_mame0220_host.md
"""
import io
import os
import sys
import json
import hashlib
import datetime
import requests

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
DEST = os.path.join(ROOT, "ref", "mame0220-win")
DOCS = os.path.join(ROOT, "docs")
UA = {"User-Agent": "Mozilla/5.0 (compatible; mame-s3/0.1)"}
BASE = "https://github.com/mamedev/mame/releases/download/mame0220/"
BIN = "mame0220b_64bit.exe"


def sha256(path):
    h = hashlib.sha256()
    with io.open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def fetch(url, path, expect_size=None):
    if os.path.exists(path) and expect_size and os.path.getsize(path) == expect_size:
        return "cached"
    tmp = path + ".part"
    with requests.get(url, headers=UA, stream=True, timeout=180) as r:
        r.raise_for_status()
        total = int(r.headers.get("Content-Length") or 0)
        done = 0
        with io.open(tmp, "wb") as f:
            for chunk in r.iter_content(1 << 20):
                f.write(chunk)
                done += len(chunk)
                if total:
                    sys.stdout.write("\r  %s: %d%% (%d/%d B)"
                                     % (os.path.basename(path), 100 * done // total,
                                        done, total))
                    sys.stdout.flush()
    sys.stdout.write("\n")
    os.replace(tmp, path)
    return "downloaded"


def main():
    os.makedirs(DEST, exist_ok=True)
    os.makedirs(DOCS, exist_ok=True)
    res = {"desc": "MAME 0.220 host binary (official release asset)",
           "generated_utc": datetime.datetime.now(datetime.timezone.utc)
           .strftime("%Y-%m-%dT%H:%M:%SZ")}

    sums_path = os.path.join(DEST, "SHA256SUMS")
    print("lade SHA256SUMS")
    fetch(BASE + "SHA256SUMS", sums_path)
    sums = io.open(sums_path, "r", encoding="utf-8", errors="replace").read()
    res["sha256sums"] = sums.strip()

    want = None
    for line in sums.splitlines():
        parts = line.split()
        if len(parts) >= 2 and BIN in parts[-1]:
            want = parts[0].lower()
    res["expected_sha256"] = want

    bin_path = os.path.join(DEST, BIN)
    print("lade %s (ca. 78 MB)" % BIN)
    how = fetch(BASE + BIN, bin_path, expect_size=77877048)
    res["download"] = how
    res["size"] = os.path.getsize(bin_path)
    got = sha256(bin_path)
    res["actual_sha256"] = got
    res["sha256_ok"] = (want is None) or (got == want)

    with io.open(os.path.join(DOCS, "07_mame0220_host.json"), "w", encoding="utf-8") as f:
        json.dump(res, f, indent=2, ensure_ascii=False)

    L = ["# 07 - MAME 0.220 Host-Fassung (offizielles Release-Asset)", "",
         "Erzeugt: %s" % res["generated_utc"],
         "Quelle: `%s%s`" % (BASE, BIN), "",
         "| Groesse | erwartete SHA256 | tatsaechliche SHA256 | Status |",
         "|---:|---|---|---|",
         "| %d B | `%s` | `%s` | **%s** |" % (
             res["size"], (want or "—")[:16] + "…", got[:16] + "…",
             "OK" if res["sha256_ok"] else "ABWEICHUNG"),
         "", "Offizielle Pruefsummendatei (`SHA256SUMS`):", "", "```", sums.strip(), "```", ""]
    with io.open(os.path.join(DOCS, "07_mame0220_host.md"), "w", encoding="utf-8",
                 newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("size=%d sha256_ok=%s" % (res["size"], res["sha256_ok"]))
    return 0 if res["sha256_ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
