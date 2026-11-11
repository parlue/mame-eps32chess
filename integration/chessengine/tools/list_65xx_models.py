"""Compare local MessChess models with the two compiled CPU device types.
Read-only executables (-listxml); no emulator/ROM execution or active rebuild.
"""
from pathlib import Path
import argparse,concurrent.futures,csv,json,subprocess,xml.etree.ElementTree as ET
p=argparse.ArgumentParser();p.add_argument('--cbemu',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[3];out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
exe=a.cbemu/'MessChess/MessChess.exe';official=root/'ref/mame0220-win/bin/mame64.exe'
source=root/'ref/mame0220-src';interfaces=a.cbemu/'MessChess/plugins/chessengine/interfaces'
xml=out/'messchess-all.xml'
if not xml.exists():
    with xml.open('wb') as f:subprocess.run([str(exe),'-noreadconfig','-listxml'],stdout=f,stderr=subprocess.PIPE,check=True,timeout=90,cwd=out)
engine_commands=(a.cbemu/'MessChess/Engines.lst').read_text(encoding='utf-8-sig').splitlines()
engine_sets={c.split()[0] for c in engine_commands if c.strip()}
doc=ET.parse(xml);machines=[m for m in doc.getroot().findall('machine') if m.get('isdevice')!='yes' and m.get('runnable')!='no' and m.get('name') in engine_sets]
supported={'MOS Technology M6502','MOS Technology M65C02'}
rows=[];excluded=[]
for m in machines:
    chips=[e for e in m.findall('chip') if e.get('type')=='cpu'];cpus={e.get('name') for e in chips}
    item={'set':m.get('name'),'name':m.findtext('description'),'cbemu_source':m.get('sourcefile'),'cpu':'; '.join(f'{e.get("name")} @ {int(e.get("clock","0"))/1e6:g} MHz' for e in chips),'cpu_names':sorted(cpus),'rom_bytes':sum(int(e.get('size','0')) for e in m.findall('rom')),'romof':m.get('romof',''),'cloneof':m.get('cloneof',''),'softwarelists':[e.get('name') for e in m.findall('softwarelist')],'cbemu_devices':[e.get('name') for e in m.findall('device_ref')],'lua':m.get('name') if (interfaces/(m.get('name')+'.lua')).exists() else m.get('cloneof') if m.get('cloneof') and (interfaces/(m.get('cloneof')+'.lua')).exists() else '', 'cbemu_archive':(a.cbemu/'MessChess/roms'/(m.get('name')+'.zip')).exists()}
    item['engine_commands']=[c for c in engine_commands if c.split()[0]==item['set']]
    if cpus and cpus<=supported:rows.append(item)
    else:excluded.append(item)
def check_original(item):
    path=out/(item['set']+'-upstream.xml')
    if not path.exists():
        r=subprocess.run([str(official),'-noreadconfig','-listxml',item['set']],capture_output=True,timeout=30,cwd=out)
        path.write_bytes(r.stdout)
    try:
        d=ET.parse(path);m=d.getroot().find(f"machine[@name='{item['set']}']")
    except ET.ParseError:m=None
    item['upstream_0220']=m is not None
    if m is not None:
        item['upstream_source']=m.get('sourcefile')
        item['upstream_name']=m.findtext('description')
        item['upstream_cpu']=[e.attrib for e in m.findall('chip') if e.get('type')=='cpu']
        item['upstream_devices']=[e.get('name') for e in m.findall('device_ref')]
        item['upstream_roms']=[e.attrib for e in m.findall('rom')]
        item['original_cpu_supported']=bool(item['upstream_cpu']) and all(e['name'] in supported for e in item['upstream_cpu'])
        item['source_present']=(source/'src/mame/drivers'/item['upstream_source']).exists()
        if item['original_cpu_supported']:
            audit=out/(item['set']+'-rom-audit.txt')
            if not audit.exists():
                r=subprocess.run([str(official),'-noreadconfig','-rompath',str(a.cbemu/'MessChess/roms'),'-verifyroms',item['set']],capture_output=True,timeout=30,cwd=out)
                audit.write_text(str(r.returncode)+'\n'+r.stdout.decode(errors='replace')+r.stderr.decode(errors='replace'),encoding='utf-8')
            audit_text=audit.read_text(encoding='utf-8');item['rom_audit_pass']=audit_text.splitlines()[0]=='0';item['rom_audit']=audit_text
    else:item['original_cpu_supported']=False
    return item
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:all_rows=list(pool.map(check_original,rows+excluded))
# Check the original CPU as well: the CB-Emu fork may have changed a CPU type.
rows=[r for r in all_rows if set(r['cpu_names'])<=supported or r.get('original_cpu_supported')]
excluded=[r for r in all_rows if r not in rows]
rows.sort(key=lambda r:r['set']);excluded.sort(key=lambda r:r['set'])
result={'supported_cpu_devices':['m6502','m65c02'],'disassemblers_are_not_additional_cpus':['m6502d','m65c02d'],'messchess_models':len(machines),'candidates':rows,'excluded':excluded}
(out/'65xx-models.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
with (out/'65xx-models.csv').open('w',encoding='utf-8-sig',newline='') as f:
    fields=['set','name','upstream_name','cpu','rom_bytes','upstream_0220','original_cpu_supported','upstream_source','lua','cbemu_archive','rom_audit_pass','romof','cloneof']
    w=csv.DictWriter(f,fieldnames=fields,extrasaction='ignore',delimiter=';');w.writeheader();w.writerows(rows)
print('CB-Emu runnable systems:',len(machines),'CPU candidates:',len(rows),'also present with supported CPU in upstream 0.220:',sum(r['upstream_0220'] and r['original_cpu_supported'] for r in rows))
from collections import defaultdict
groups=defaultdict(list)
for r in rows:groups[r.get('upstream_source',r['cbemu_source'])].append(r)
for group,rr in sorted(groups.items()):print(group,':',','.join(r['set']+(' [CB-only]' if not r['upstream_0220'] else '') for r in rr))
eligible=[r for r in rows if r['upstream_0220'] and r['original_cpu_supported']]
extra=[r for r in rows if not r['upstream_0220']]
lines=['# MessChess: ROM-/Modellliste für M6502 und M65C02','',
'Stand: 10. November 2026. Ermittelt aus der lokalen MessChess-Installation und einzeln gegen die offizielle MAME-0.220-EXE abgeglichen. Kein Modell wurde für diese Liste auf dem ESP gestartet.','',
f'**{len(eligible)} Set-IDs im Original-MAME 0.220 passen zu den zwei CPU-Kernen. {len(extra)} weitere CB-Emu-Set-IDs passen zur CPU, fehlen aber im Original-0.220.** `m6502d` und `m65c02d` sind Disassembler, keine weiteren CPU-Typen.','',
'Der vorhandene ESP-Startpfad registriert momentan nur `mm4`. Die folgende Liste beschreibt mögliche Treibererweiterungen, keine bereits spielbaren ESP-Modelle.','',
'## 1. Mit dem bereits übersetzten Treiber mephisto_mm2.cpp erreichbar','',
'Diese 16 Set-IDs liegen alle im vorhandenen Quelltreiber. Für ihre Auswahl muss die Treibertabelle ergänzt werden; Lua-Profile und ROM-Sätze müssen jeweils passen. Nur die MM-IV-/MM-V-Standardinterfaces sind bisher als ESP-Modul portiert.','',
'| Set | Modell laut Original-0.220 | ROM-Prüfung der vorhandenen CB-Emu-Dateien |','|---|---|---|']
for r in eligible:
    if r['upstream_source']=='mephisto_mm2.cpp':lines.append(f'| `{r["set"]}` | {r["upstream_name"]} | '+('OK / best available' if r['rom_audit_pass'] else '**nicht vollständig passend**')+' |')
lines+=['','Turbo-Takte sind kein Leistungsnachweis auf dem ESP. Der MM-IV-TurboKit verlangt beispielsweise 18 MHz emulierten CPU-Takt.','',
'## 2. Weitere 0.220-Treiber, CPU-seitig passend','',
'Zusätzlich zum genannten Treiber werden jeweils dessen Peripherie und Lua-Interface benötigt.','',
'| Quelltreiber | Set-IDs | Zusätzliche Gerätetypen gegenüber mm4 (aus -listxml) |','|---|---|---|']
baseline=set(next(r for r in eligible if r['set']=='mm4')['upstream_devices'])
for group,rr in sorted(groups.items()):
    rr=[r for r in rr if r in eligible and r['upstream_source']!='mephisto_mm2.cpp']
    if not rr:continue
    devices=sorted(set(d for r in rr for d in r['upstream_devices'])-baseline)
    lines.append(f'| `{group}` | '+', '.join('`'+r['set']+'`' for r in rr)+' | '+(', '.join('`'+d+'`' for d in devices) or 'keine zusätzlichen Typen; eigener Treiber und Verdrahtung bleiben nötig')+' |')
lines+=['','## 3. Vollständige Original-0.220-Modellliste','',
'ROM-Status stammt von `mame64 -noreadconfig -rompath <CB-Emu/MessChess/roms> -verifyroms <set>`. `best available` kann bekannte BAD_DUMP-Warnungen enthalten. Ein fehlendes eigenes ZIP ist bei einem zusammengefassten Parent-Archiv kein Fehler. Ein bestandener Audit beweist keine ESP-Lauffähigkeit.','',
'| Set | Modell (Original-0.220) | CPU / Takt (Original-0.220) | ROM-Audit |','|---|---|---|---|']
for r in eligible:
    cpus=', '.join(e['name']+' @ '+f'{int(e.get("clock","0"))/1e6:g}'+' MHz' for e in r['upstream_cpu'])
    status='OK / best available' if r['rom_audit_pass'] else '**fehlt / abweichend**'
    if not r['upstream_roms']:status+='; **Cartridge separat nötig**'
    lines.append(f'| `{r["set"]}` | {r["upstream_name"]} | {cpus} | {status} |')
failed=[r['set'] for r in eligible if not r['rom_audit_pass']]
lines+=['',f'ROM-Audit: **{len(eligible)-len(failed)}/{len(eligible)} bestanden**, fehlend/abweichend: '+(', '.join('`'+s+'`' for s in failed) or 'keine')+'.','',
'`arb` und `ggm` sind Grundgeräte mit Softwaremodulen. Ihr Basis-Audit bestätigt nicht, dass das ausgewählte Cartridge vorhanden oder anschließbar ist. MessChess-Startprofile:', '']
for r in eligible:
    if not r['upstream_roms']:lines.append('- '+', '.join('`'+c+'`' for c in r['engine_commands']))
lines+=['','## 4. Zusätzliche CB-Emu-Set-IDs, nicht im Original-0.220','',
'Diese Set-IDs fehlen im Original oder wurden anders benannt. Erst ROM-Hashes und Hardware vergleichen: passende Original-Sets zuordnen oder fehlende Treiberänderungen portieren. ROMs allein ergänzen keinen fehlenden Treiber.','',
'| Set | CB-Emu-Modell | Quellfamilie laut CB-Emu |','|---|---|---|']
for r in extra:lines.append(f'| `{r["set"]}` | {r["name"]} | `{r["cbemu_source"]}` |')
lines+=['','## 5. Nicht durch die vorhandenen zwei CPU-Kerne abgedeckt','',
'Insbesondere Rockwell `R65C02`, `M65SC02`, MOS `M6504` und weitere 65xx-Derivate sind eigenständige MAME-Gerätetypen. Nicht als kompatibel mitrechnen, auch wenn Teile des Befehlssatzes ähnlich sind. Andere Architekturen (Z80, 68000/020/030/040, ARM, F8, H8, 6301/6303, 1802 usw.) bleiben ebenfalls außerhalb dieses Ports.','',
'Beispiele aus dem vorhandenen Katalog:','',
'| CPU-Klasse | Zahl betroffener Modell-Set-IDs (Mehrfach-CPU-Geräte können mehrfach zählen) | Beispiele |','|---|---:|---|']
from collections import Counter
counts=Counter(c for r in excluded for c in set(r['cpu_names']))
for cpu,count in counts.most_common(18):
    samples=[r['set'] for r in excluded if cpu in r['cpu_names']][:6]
    lines.append(f'| {cpu} | {count} | '+', '.join('`'+s+'`' for s in samples)+' |')
lines+=['','## Daten und Einschränkungen','',
'- `65xx-models.csv`: alle CPU-passenden Kandidaten mit beiden Versionsnamen und Audit-Status.','- `65xx-models.json`: zusätzlich vollständige Geräte-/CPU-/ROM-Metadaten, Startprofile und ausgeschlossene Systeme.','- Pro Modell liegen Original-XML und ROM-Audit daneben.','- XML-ROM-Größen im JSON/CSV sind deklarierte Dateisummen, kein RAM-Bedarf; BIOS-Alternativen und Zusatz-ROMs können enthalten sein.','- Modellnamen und ROM-Namen können zwischen CB-Emu und dem Original abweichen. Deshalb ist der Original-Audit entscheidend, nicht nur der Set-Name.','- Alle Daten stammen aus den lokalen Programmdateien und Quellen; keine ROM-Dateien wurden in diese Dokumentation kopiert.','']
(out/'65xx-modelle.md').write_text('\n'.join(lines),encoding='utf-8')
print('ROM audits:',len(eligible)-len(failed),'/',len(eligible),'PASS; failed:',','.join(failed))
