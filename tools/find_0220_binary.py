import io, json
import requests

UA = {"User-Agent": "Mozilla/5.0 (compatible; mame-s3/0.1)"}
out = {}

def probe(u, headers=None):
    h = dict(UA)
    if headers:
        h.update(headers)
    try:
        r = requests.get(u, headers=h, timeout=30, stream=True, allow_redirects=True)
        info = {"status": r.status_code, "ctype": r.headers.get("Content-Type"),
                "clen": r.headers.get("Content-Length"),
                "final": r.url[:160]}
        # kleine Kostprobe fuer Textantworten
        if (r.headers.get("Content-Type") or "").startswith(("text", "application/json")):
            body = r.raw.read(4000)
            info["head"] = body.decode("utf-8", "replace")[:1200]
        r.close()
        return info
    except Exception as e:
        return {"error": repr(e)}

# 1) GitHub: hat das Release-Assets?
try:
    r = requests.get("https://api.github.com/repos/mamedev/mame/releases/tags/mame0220",
                     headers=UA, timeout=30)
    j = r.json()
    out["gh_release"] = {"status": r.status_code, "name": j.get("name"),
                         "assets": [{"name": a.get("name"), "size": a.get("size"),
                                     "url": a.get("browser_download_url")}
                                    for a in j.get("assets", [])]}
except Exception as e:
    out["gh_release"] = {"error": repr(e)}

# 2) SourceForge-Spiegel
out["sf_folder"] = probe("https://sourceforge.net/projects/mame/files/mame/0.220/")
out["sf_dl_64"] = probe("https://master.dl.sourceforge.net/project/mame/mame/0.220/mame0220b_64bit.exe")
out["sf_web_64"] = probe("https://sourceforge.net/projects/mame/files/mame/0.220/mame0220b_64bit.exe/download")
out["sf_src_zip"] = probe("https://master.dl.sourceforge.net/project/mame/mame/0.220/mame0220s.exe")

# 3) Progetto-Snaps
out["snaps"] = probe("https://www.progettosnaps.net/mame/")

with io.open("_get220.json", "w", encoding="utf-8") as f:
    json.dump(out, f, indent=2, ensure_ascii=False)
print("END")
