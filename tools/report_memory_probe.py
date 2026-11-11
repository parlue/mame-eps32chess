"""Validate startup probe checksums/repetitions and summarize harness timings.

These timings include harness dispatch and preemption, not live CPU shares.
"""
import argparse,hashlib,json,re,statistics
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('log',type=Path)
p.add_argument('--output',type=Path,required=True)
a=p.parse_args()
raw=a.log.read_bytes()
text=raw.decode('utf-16') if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else raw.decode('utf-8')
drivers={};driver=None
pattern=re.compile(r'^\[MAME MEMORY PROBE\] pattern=(ordered|shuffled) method=(\w+) round=(\d+) reads=(\d+) us=(\d+) checksum=(\d+)$')
for line in text.splitlines():
    m=re.match(r'^\[RETRO\] load (mm4|mm5);',line)
    if m:
        driver=m[1];assert driver not in drivers,'Use one load per model'
        drivers[driver]={'records':[],'pass':0}
    if '[MAME MEMORY PROBE]' not in line:continue
    assert driver,'Probe has no model load marker'
    if line=='[MAME MEMORY PROBE] PASS read_only=1 checksums=OK':drivers[driver]['pass']+=1
    m=pattern.match(line)
    if m:drivers[driver]['records'].append({'pattern':m[1],'method':m[2],'round':int(m[3]),'reads':int(m[4]),'us':int(m[5]),'checksum':int(m[6])})
assert set(drivers)=={'mm4','mm5'},'Both ROM models are required'
for name,data in drivers.items():
    assert data['pass']==1 and len(data['records'])==36
    assert len({r['checksum'] for r in data['records']})==1
    data['summary']={}
    for order in ['ordered','shuffled']:
        data['summary'][order]={}
        for method in ['raw','cache','pages','virtual_data','virtual_operand','virtual_opcode']:
            rows=[r for r in data['records'] if r['pattern']==order and r['method']==method]
            assert {r['round'] for r in rows}=={0,1,2} and len(rows)==3
            assert all(r['reads']==16384 and r['us']>0 for r in rows)
            costs=[r['us']*1000/r['reads'] for r in rows]
            stats={'median_ns_per_read':statistics.median(costs),'min_ns_per_read':min(costs),'max_ns_per_read':max(costs)}
            data['summary'][order][method]=stats
            print(name,order,method,round(stats['median_ns_per_read'],1),'ns/read (includes harness)')
a.output.write_text(json.dumps({'log_sha256':hashlib.sha256(raw).hexdigest(),'limits':__doc__,'drivers':drivers},indent=2)+'\n')
print('PASS: both models, 72 timing blocks, all checksums and repetitions verified')
