import io, json, os, subprocess
import requests

UA = {"User-Agent": "Mozilla/5.0 (mame-s3 probe; local)"}
out = {}

# --- 1) lokale MSVC/MSYS2-Kandidaten
cands = [
    r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    r"C:\msys64\usr\bin\make.exe",
    r"C:\msys64\mingw64\bin\g++.exe",
    r"C:\Program Files\Git\usr\bin\make.exe",
]
out["local"] = {c: os.path.exists(c) for c in cands}
out["git_bash_usr_bin"] = os.path.isdir(r"C:\Program Files\Git\usr\bin")
try:
    gb = os.listdir(r"C:\Program Files\Git\usr\bin")
    out["git_usr_bin_has"] = [n for n in gb if n.lower() in ("make.exe", "sh.exe", "gcc.exe", "g++.exe")]
except Exception as e:
    out["git_usr_bin_has"] = repr(e)

# --- 2) Erreichbarkeit der MAME-Downloads
urls = [
    "https://www.mamedev.org/oldrel.html",
    "https://download.mamedev.org/releases/mame0220b_64bit.exe",
    "https://download.mamedev.org/releases/mame0220b_32bit.exe",
    "https://github.com/mamedev/mame/releases/tag/mame0220",
    "https://msys2.org/",
    "https://repo.msys2.org/distrib/x86_64/",
]
res = {}
for u in urls:
    try:
        r = requests.get(u, headers=UA, timeout=25, stream=True)
        res[u] = {"status": r.status_code,
                  "len": r.headers.get("Content-Length"),
                  "ctype": r.headers.get("Content-Type")}
        r.close()
    except Exception as e:
        res[u] = {"error": repr(e)}
out["urls"] = res

with io.open("_dl.txt", "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, ensure_ascii=False)
print("END")
