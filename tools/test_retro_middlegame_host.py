"""Validate legal replay and actual ROM replies on official PC MAME 0.220.

This verifies bridge behaviour, not ESP performance or internal ROM board RAM.
Build retro_position_check.exe with build_retro_position_check.ps1 first.
"""
import argparse,json,os,re,subprocess,time
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
root=Path(__file__).resolve().parents[1]
module=root/'integration/chessengine'
checker=module/'build-host/position-check/retro_position_check.exe'
cases=json.loads((root/'tools/retro_benchmark_positions.json').read_text())
results={}
for name,case in cases.items():
    checked=subprocess.run([str(checker)],input=case['moves'],text=True,capture_output=True,check=True)
    position=json.loads(checked.stdout)
    assert position['plies']==32 and len(position['legal'])>20
    results[name]={'position':position,'roms':{}}
    for driver in ['mm4','mm5']:
        work=a.output/(name+'-'+driver);work.mkdir()
        env={**os.environ,'RETRO_TEST_MODULE':module.as_posix(),'RETRO_TEST_MOVES':case['moves'],
             'RETRO_TEST_LEVEL':'le 1','RETRO_TEST_LIMIT':'400'}
        command=[str(root/'ref/mame0220-win/bin/mame64.exe'),driver,'-noreadconfig','-noplugins',
                 '-video','none','-sound','none','-nothrottle','-skip_gameinfo',
                 '-rompath',str(module/'local-roms'),'-autoboot_script',str(module/'tests/rom_host.lua'),
                 '-autoboot_delay','0','-seconds_to_run','410']
        start=time.monotonic()
        run=subprocess.run(command,cwd=work,env=env,capture_output=True,timeout=180)
        log=(run.stdout+run.stderr).decode('utf-8',errors='replace')
        (work/'rom.log').write_text(log,encoding='utf-8')
        assert run.returncode==0 and '[RETRO ROM FAIL]' not in log,log[-2000:]
        assert '[RETRO LOG] Game edit is complete!' in log, 'Replay did not finish'
        assert '[RETRO POSITION READY] '+case['moves'] in log, 'Missing replay readiness'
        match=re.search(r'\[RETRO ROM PASS\] '+driver+r' bestmove ([a-h][1-8][a-h][1-8][qrbn]?)',log)
        assert match and match[1] in position['legal'],'ROM reply illegal in expected middlegame'
        results[name]['roms'][driver]={'bestmove':match[1],'wall_seconds':round(time.monotonic()-start,3)}
        print('PASS:',name,driver,match[1],position['fen'],flush=True)
        (a.output/'evidence.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS: both 32-ply positions and legal MM IV/MM V ROM replies on PC; not ESP speed')
