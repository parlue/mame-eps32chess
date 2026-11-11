import io, json, os, re
import requests

UA = {"User-Agent": "mame-s3-analysis/0.1 (+local research)"}
REF = "mame0220"
API = "https://api.github.com/repos/mamedev/mame/contents/%s?ref=%s"

CHESS_RE = re.compile(r'(mephisto|fidelity|novag|chess|saitek|conchess|tasc|mchess|cchess|rebel|meph|mk1|mk2|modular)', re.I)

out = {}


def listing(path):
    r = requests.get(API % (path, REF), headers=UA, timeout=30)
    if r.status_code != 200:
        return {"error": r.status_code, "body": r.text[:200]}
    return [x["name"] for x in r.json()]


# 1) Struktur von src/mame
top = listing("src/mame")
out["src_mame_dirs"] = top if isinstance(top, dict) else [n for n in top if "." not in n][:40]
out["src_mame_flatfiles"] = top if isinstance(top, dict) else [n for n in top if n.endswith(".cpp")][:10]

# 2) Treiberordner: 0.220 nutzt noch src/mame/drivers
for cand in ("src/mame/drivers", "src/mame/chess"):
    got = listing(cand)
    if isinstance(got, dict) and got.get("error"):
        out["dir_%s" % cand] = got
    else:
        out["dir_%s" % cand] = sorted(n for n in got if CHESS_RE.search(n))

# 3) Plugins im Upstream (existiert das chessengine-Plugin hier?)
out["plugins"] = listing("plugins")

io.open(".ghlist.json", "w", encoding="utf-8").write(json.dumps(out, indent=2, ensure_ascii=False))
print("END")
