"""Compile a separate archive with the existing S3 compiler, without rebuilding
or modifying the active MAME build. Reuse its include paths for API checking.
"""
from pathlib import Path
import concurrent.futures,json,subprocess
root=Path(__file__).resolve().parents[1];mame=root.parents[1]
database=mame/'esp32s3/.pio/build/esp32s3-skeleton/compile_commands.json'
commands=json.loads(database.read_text());entry=next(e for e in commands if e['file'].replace('\\','/').endswith('/s3_startup.cpp'))
out=root/'build-xtensa';out.mkdir(exist_ok=True)
prefix=entry['command'].rsplit(' -o ',1)[0]
prefix=prefix.replace('-D__UINT32_TYPE__=unsigned int','"-D__UINT32_TYPE__=unsigned int"')
includes=f' -I"{root / "include"}" -I"{root / "generated"}" -I"{mame / "ref/mame0220-src/3rdparty/lua/src"}"'
def cpp(source,target,syntax=False):
    cmd=prefix+includes+(' -fsyntax-only ' if syntax else f' -Os -ffunction-sections -fdata-sections -o "{target}" -c ')+f'"{source}"'
    r=subprocess.run(cmd,cwd=entry['directory'],capture_output=True)
    if r.returncode:raise RuntimeError(r.stdout.decode(errors='replace')+r.stderr.decode(errors='replace'))
    return r.stderr.decode(errors='replace')
probe=out/'mame_api_check.cpp';probe.write_text('#include "retro_mame_machine.h"\n',encoding='utf-8')
log=cpp(probe,None,True)
log+=cpp(root/'src/retro_runtime.cpp',out/'retro_runtime.o')
compiler=Path(entry['command'].split(' ',1)[0].strip('"'));compiler=compiler.with_name(compiler.name.replace('g++','gcc'))
lua=mame/'ref/mame0220-src/3rdparty/lua/src'
exclude={'lua','luac','linit','liolib','loslib','loadlib','ldblib'}
sources=[s for s in lua.glob('*.c') if s.stem not in exclude]
def c(source):
    r=subprocess.run([str(compiler),'-Os','-ffunction-sections','-fdata-sections','-mlongcalls','-I'+str(lua),'-c',str(source),'-o',str(out/(source.stem+'.o'))],capture_output=True)
    if r.returncode:raise RuntimeError(r.stderr.decode(errors='replace'))
    return r.stderr.decode(errors='replace')
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    for warnings in pool.map(c,sources):log+=warnings
ar=compiler.with_name(compiler.name.replace('gcc','ar'))
objects=[out/(s.stem+'.o') for s in sources]+[out/'retro_runtime.o']
subprocess.run([str(ar),'rcs',str(out/'libretro_uci.a'),*map(str,objects)],check=True)
size=compiler.with_name(compiler.name.replace('gcc','size'))
log+=subprocess.check_output([str(size),'-t',*map(str,objects)],text=True)
(out/'validation.txt').write_text(log,encoding='utf-8')
print('PASS: adapter checked against actual MAME 0.220 headers; runtime +',len(sources),'Lua sources compiled for ESP32-S3.')
print(log.splitlines()[-1]);print(out/'libretro_uci.a')
