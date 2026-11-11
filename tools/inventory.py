#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
inventory.py - Inventar des vom Auftraggeber bereitgestellten Pakets.

Liest rekursiv mame-s3/incoming/ (nur lesend) und berichtet:
  * gefundene MAME/MESS-Binaries + daraus gelesene Versionsstrings
  * chessengine-Plugin (init.lua, plugin.json, interfaces/*.lua)
  * Startskripte/Configs (bat, cmd, sh, ini, cfg, ps1)
  * ROM-Dateien und ROM-Archive; fuer Archive werden die CRC32 aus dem
    ZIP-Zentralverzeichnis gelesen (OHNE Inhalte zu extrahieren)
  * Abgleich gegen docs/03_rom_manifest.json (Dateiname, Groesse, CRC32)

Es werden keine Dateien veraendert und keine ROM-Inhalte kopiert oder
ausgegeben - nur Metadaten. Ausgabe: mame-s3/incoming_inventory.md/.json
"""
import io
import os
import re
import json
import zlib
import hashlib
import zipfile
import datetime

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
INCOMING = os.path.join(ROOT, "incoming")
MANIFEST = os.path.join(ROOT, "docs", "03_rom_manifest.json")

ROM_EXT = {".bin", ".rom", ".zip", ".7z", ".rar", ".chd", ".dat", ".u1", ".u2",
           ".u3", ".u4", ".hi", ".lo", ".ic", ".1", ".2", ".3", ".4", ".5", ".6"}
SCRIPT_EXT = {".bat", ".cmd", ".sh", ".ps1", ".ini", ".cfg", ".conf", ".lua"}
BIN_RE = re.compile(rb'mame|mess', re.I)
VER_RE = re.compile(rb'0\.2\d\d')


def sha1_of(path, chunk=1 << 20):
    h = hashlib.sha1()
    with io.open(path, "rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def crc_of(path, chunk=1 << 20):
    c = 0
    with io.open(path, "rb") as f:
        while True:
            b = f.read(chunk)
            if not b:
                break
            c = zlib.crc32(b, c)
    return c & 0xFFFFFFFF


def version_hits(path, limit=8 << 20):
    """Haeufigste 0.2xx-Strings in der Datei (Versionserkennung)."""
    out = {}
    try:
        with io.open(path, "rb") as f:
            data = f.read(limit)
        for m in VER_RE.findall(data):
            s = m.decode("ascii", "replace")
            out[s] = out.get(s, 0) + 1
    except Exception:
        pass
    return dict(sorted(out.items(), key=lambda kv: -kv[1])[:6])


def expected_roms():
    if not os.path.exists(MANIFEST):
        return {}
    with io.open(MANIFEST, "r", encoding="utf-8") as f:
        m = json.load(f)
    exp = {}
    for setname, e in m.get("sets", {}).items():
        if not e.get("found"):
            continue
        for r in e["roms"]:
            fn = r.get("file")
            if not fn or not r.get("crc"):
                continue
            # ROM_CONTINUE/RELOAD-Eintraege tragen keinen eigenen CRC
            if r["crc"].lower() in ("00000000",):
                continue
            exp.setdefault(fn, []).append({"set": setname,
                                           "crc": int(r["crc"], 16),
                                           "size": r.get("size")})
    return exp


def main():
    from collections import Counter
    out = {"generated_utc": datetime.datetime.now(datetime.timezone.utc)
           .strftime("%Y-%m-%dT%H:%M:%SZ"),
           "incoming": os.path.relpath(INCOMING, ROOT),
           "exists": os.path.isdir(INCOMING)}
    files = []
    if out["exists"]:
        for dirpath, _, filenames in os.walk(INCOMING):
            for fn in filenames:
                files.append(os.path.join(dirpath, fn))

    ext_hist = Counter()
    binaries, scripts, plugin_files = [], [], []
    roms, zip_members = [], {}
    iface_count = 0
    for p in files:
        rel = os.path.relpath(p, INCOMING)
        base = os.path.basename(p)
        ext = os.path.splitext(base)[1].lower()
        ext_hist[ext or "(none)"] += 1
        if ext in (".exe", ".dll") and BIN_RE.search(base.encode()):
            binaries.append({"path": rel, "size": os.path.getsize(p),
                             "version_hits": version_hits(p)})
        elif ext == ".zip":
            try:
                with zipfile.ZipFile(p) as z:
                    for i in z.infolist():
                        zip_members.setdefault(i.filename, []).append(
                            {"zip": rel, "crc": i.CRC & 0xFFFFFFFF, "size": i.file_size})
            except Exception as e:
                zip_members.setdefault("(unreadable: %s)" % rel, []).append({"error": repr(e)})
        if ext in SCRIPT_EXT:
            scripts.append({"path": rel, "size": os.path.getsize(p)})
        if base.lower() in ("init.lua", "plugin.json"):
            plugin_files.append(rel)
        if "interfaces" in rel.replace("\\", "/").lower() and ext == ".lua":
            iface_count += 1
        if ext in ROM_EXT or re.fullmatch(r'\d+', base):
            roms.append({"path": rel, "size": os.path.getsize(p),
                         "crc32": "%08x" % crc_of(p), "sha1": sha1_of(p)})

    # ROM-Abgleich gegen die 0.220-Treiberdaten
    exp = expected_roms()
    checks = []
    for fn, entries in sorted(exp.items()):
        want = entries[0]
        hit = None
        for r in roms:
            if os.path.basename(r["path"]).lower() == fn.lower():
                hit = {"kind": "loose", "where": r["path"], "crc": int(r["crc32"], 16)}
                break
        if hit is None:
            for mname, ms in zip_members.items():
                if os.path.basename(mname).lower() == fn.lower() and ms and "crc" in ms[0]:
                    hit = {"kind": "zip", "where": ms[0]["zip"], "crc": ms[0]["crc"]}
                    break
        state = "MISSING"
        if hit:
            state = "OK" if hit["crc"] == want["crc"] else "CRC-MISMATCH"
        checks.append({"rom_file": fn, "sets": sorted({e["set"] for e in entries}),
                       "expected_crc32": "%08x" % want["crc"],
                       "found": hit, "state": state})

    out.update({"file_count": len(files), "ext_hist": dict(ext_hist.most_common(20)),
                "binaries": binaries, "plugin_files": plugin_files,
                "interface_files": iface_count, "scripts": scripts[:60],
                "roms": roms[:80], "zip_member_count": len(zip_members),
                "rom_checks": checks,
                "rom_summary": dict(Counter(c["state"] for c in checks))})
    with io.open(os.path.join(ROOT, "incoming_inventory.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, indent=2, ensure_ascii=False)

    L = ["# Inventar des bereitgestellten Pakets", "",
         "Erzeugt: %s" % out["generated_utc"],
         "Ordner: `%s` (%s)" % (out["incoming"], "vorhanden" if out["exists"] else "FEHLT"),
         "Dateien: %d" % len(files), "",
         "| Endung | Anzahl |", "|---|---:|"]
    for k, v in out["ext_hist"].items():
        L.append("| `%s` | %d |" % (k, v))
    L += ["", "## MAME/MESS-Binaries", ""]
    if binaries:
        L += ["| Datei | Groesse | erkannte Versionsstrings |", "|---|---:|---|"]
        for b in binaries:
            L.append("| `%s` | %d | %s |" % (b["path"], b["size"],
                     ", ".join("%s (%dx)" % (k, v) for k, v in b["version_hits"].items()) or "—"))
    else:
        L.append("_keine gefunden_")
    L += ["", "## chessengine-Plugin", "",
          "* `init.lua`/`plugin.json`: %s" % (", ".join(plugin_files) or "**fehlt**"),
          "* Interface-Dateien (`interfaces/*.lua`): **%d**" % iface_count, "",
          "## Startskripte/Configs (%d)" % len(scripts), ""]
    for s in scripts[:40]:
        L.append("* `%s` (%d B)" % (s["path"], s["size"]))
    L += ["", "## ROM-Abgleich gegen MAME 0.220 (CRC32 aus dem ZIP-Verzeichnis)", "",
          "Zusammenfassung: %s" % json.dumps(out["rom_summary"]), "",
          "| ROM-Datei | Sets | erwartete CRC32 | gefunden | Status |",
          "|---|---|---|---|---|"]
    for c in checks:
        f = c["found"]
        loc = "—" if not f else ("%s:%s" % (f["kind"], f["where"]))
        L.append("| `%s` | %s | `%s` | %s | **%s** |" % (
            c["rom_file"], ", ".join(c["sets"][:4]), c["expected_crc32"], loc, c["state"]))
    L += ["", "## Lose ROM-Dateien (%d)" % len(roms), ""]
    for r in roms[:40]:
        L.append("* `%s` (%d B, crc32 `%s`)" % (r["path"], r["size"], r["crc32"]))
    with io.open(os.path.join(ROOT, "incoming_inventory.md"), "w", encoding="utf-8",
                 newline="\n") as f:
        f.write("\n".join(L) + "\n")
    print("files=%d bins=%d ifaces=%d roms=%d checks=%s"
          % (len(files), len(binaries), iface_count, len(roms), out["rom_summary"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
