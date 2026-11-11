"""Prepare local MM IV/V ROM folders for the original MAME 0.220 driver.

Select by size, CRC32 AND SHA1; filenames can differ between CB-Emu and MAME.
Existing different files are never overwritten. Does not touch the ESP or VFS.
"""
import argparse,hashlib,json,zipfile,zlib
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--cbemu',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--machine',action='append',choices=['mm4','mm5'],required=True);a=p.parse_args()
specs={'mm4':('710',32768,'f68a4124','d1d03a9aacc291d5cb720d2ee2a209eeba13a36c'),'mm5':('mephisto5.rom',32768,'89c3d9d2','77cd6f8eeb03c713249db140d2541e3264328048')}
manifest=[]
for machine in dict.fromkeys(a.machine):
    target,size,crc,sha=specs[machine];archive=a.cbemu/'MessChess/roms'/(machine+'.zip');matches=[]
    with zipfile.ZipFile(archive) as z:
        for entry in z.infolist():
            if entry.file_size!=size or f'{entry.CRC:08x}'!=crc:continue
            data=z.read(entry)
            if hashlib.sha1(data).hexdigest()==sha and f'{zlib.crc32(data):08x}'==crc:matches.append((entry.filename,data))
    if len(matches)!=1:raise SystemExit(f'{machine}: expected exactly one verified ROM, found {len(matches)}')
    source,data=matches[0];dest=a.output/machine/target
    dest.parent.mkdir(parents=True,exist_ok=True)
    if dest.exists() and dest.read_bytes()!=data:raise SystemExit(f'Refusing to overwrite different file: {dest}')
    if not dest.exists():dest.write_bytes(data)
    manifest.append(dict(driver=machine,source_archive=str(archive),source_entry=source,target=f'{machine}/{target}',bytes=size,crc32=crc,sha1=sha,sha256=hashlib.sha256(data).hexdigest()))
(a.output/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
for entry in manifest:print(f'{entry["driver"]}: {entry["source_entry"]} -> {entry["target"]}; {entry["bytes"]} bytes, CRC+SHA1 OK')
