import io, json, os

HOME = os.path.expanduser("~")
PLAT = os.path.join(HOME, ".platformio", "platforms")
PKG = os.path.join(HOME, ".platformio", "packages")
out = {"platforms": {}, "installed": {}, "verdict": {}}

# installierte Pakete mit Version
for name in sorted(os.listdir(PKG)):
    p = os.path.join(PKG, name)
    if not os.path.isdir(p):
        continue
    v = "?"
    m = os.path.join(p, ".piopm")
    if os.path.exists(m):
        try:
            v = json.load(io.open(m, encoding="utf-8")).get("version", "?")
        except Exception:
            pass
    out["installed"][name] = v

# Plattformen: welche Pakete verlangen sie?
for name in sorted(os.listdir(PLAT)):
    p = os.path.join(PLAT, name)
    pj = os.path.join(p, "platform.json")
    if not os.path.exists(pj):
        continue
    try:
        d = json.load(io.open(pj, encoding="utf-8"))
    except Exception as e:
        out["platforms"][name] = {"error": repr(e)}
        continue
    pk = d.get("packages", {})
    entry = {"version": d.get("version"), "packages": {}}
    for k, v in pk.items():
        val = v.get("version") if isinstance(v, dict) else v
        entry["packages"][k] = val
    # nur die fuer uns interessanten Pakete
    keep = {k: v for k, v in entry["packages"].items()
            if any(s in k for s in ("toolchain", "framework-espidf", "tool-idf",
                                    "tool-cmake", "tool-ninja", "tool-scons"))}
    entry["packages_relevant"] = keep
    out["platforms"][name] = entry

# Verdikt: passen die gecachten Toolchains zu einer der Plattformen?
def need(entry, key):
    for k, v in entry["packages_relevant"].items():
        if key in k:
            return v
    return None

for name, e in out["platforms"].items():
    if "packages_relevant" not in e:
        continue
    xt = need(e, "toolchain-xtensa-esp32s3") or need(e, "toolchain-xtensa-esp-elf")
    rv = need(e, "toolchain-riscv32-esp")
    have_xt = [k for k in out["installed"] if k.startswith("toolchain-xtensa")]
    have_rv = [k for k in out["installed"] if k.startswith("toolchain-riscv32")]
    out["verdict"][name] = {
        "platform_version": e["version"],
        "needs_xtensa": xt, "have_xtensa": have_xt,
        "needs_riscv32": rv, "have_riscv32": have_rv,
        "offline_ok_if_versions_match": None,
    }

io.open("_pio_probe.json", "w", encoding="utf-8").write(
    json.dumps(out, indent=2, ensure_ascii=False))
print("END")
