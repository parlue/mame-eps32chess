"""Test the ported bridge with real MM IV/V ROMs using official PC MAME 0.220.
This is not an ESP performance or hardware test. Does not flash any device.
"""
from pathlib import Path
import os,subprocess,time
root=Path(__file__).resolve().parents[1];mame=root.parents[1]
out=root/'build-host/rom-tests';out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['RETRO_TEST_MODULE']=str(root).replace('\\','/')
for driver in ['mm4','mm5']:
    work=out/driver;work.mkdir(exist_ok=True)
    command=[str(mame/'ref/mame0220-win/bin/mame64.exe'),driver,'-noreadconfig','-noplugins','-video','none','-sound','none','-nothrottle','-skip_gameinfo','-rompath',str(root/'local-roms'),'-autoboot_script',str(root/'tests/rom_host.lua'),'-autoboot_delay','0','-seconds_to_run','130']
    start=time.monotonic()
    r=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=90)
    text=(r.stdout+r.stderr).decode('utf-8',errors='replace')
    (out/(driver+'.log')).write_text(text,encoding='utf-8')
    print(driver,'exit',r.returncode,'wall seconds',round(time.monotonic()-start,2))
    print('\n'.join(s for s in text.splitlines() if 'RETRO' in s))
    if r.returncode or '[RETRO ROM PASS]' not in text or '[RETRO ROM FAIL]' in text:raise SystemExit('ROM host verification failed: '+driver)
print('PASS: actual MM IV and MM V ROMs answered through the ported Lua bridge on original PC MAME 0.220.')
