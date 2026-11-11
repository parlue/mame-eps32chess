"""Syntax-check the local platform changes with the existing actual compiler.
Does not modify build configuration or objects used by a running build.
"""
from pathlib import Path
import json,subprocess
root=Path(__file__).resolve().parents[1]
database=root/'esp32s3/.pio/build/esp32s3-skeleton/compile_commands.json'
entries=json.loads(database.read_text())
log=[]
for name in ['s3_startup.cpp','s3_osd.cpp','s3_http.cpp','s3_osd_output.cpp','s3_netdev.cpp','s3_uci_console.cpp']:
    if name == 's3_uci_console.cpp':
        entry=next(e for e in entries if Path(e['file']).name=='s3_startup.cpp').copy()
        entry['file']=str(root/'integration/chessengine/src'/name)
        entry['command']+=' -I"'+str(root/'integration/chessengine/include')+'"'
    else:
        entry=next(e for e in entries if Path(e['file']).name==name)
    prefix=entry['command'].rsplit(' -o ',1)[0].replace('-D__UINT32_TYPE__=unsigned int','"-D__UINT32_TYPE__=unsigned int"')
    if name=='s3_uci_console.cpp':
        prefix+=' -I"'+str(root/'integration/chessengine/include')+'"'
    command=prefix+' -fsyntax-only "'+entry['file']+'"'
    r=subprocess.run(command,cwd=entry['directory'],capture_output=True)
    log.append(name+' exit='+str(r.returncode)+'\n'+(r.stdout+r.stderr).decode(errors='replace'))
    if r.returncode:
        print(log[-1]);raise SystemExit(r.returncode)
    print(name+': PASS')
(root/'docs/takeover-platform-syntax.log').write_text('\n'.join(log),encoding='utf-8')
