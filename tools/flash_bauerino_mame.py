"""Validate the combined application and preserved images; optional guarded flash."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--write',action='store_true')
p.add_argument('--port',default='COM8')
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
workspace=root.parent
build=root/'bauerino/.pio/build/bauerino480'
backup=root/'backups/takeover-20261110/waveshare-before-mame-16MB.bin'
assert backup.stat().st_size==0x1000000
assert hashlib.sha256(backup.read_bytes()).hexdigest()=='4264eb20d86cfd878d9135853a6b27c1ec817379fa63fbfc15245250c38a8e1e'

parts={}
table=build/'partitions.bin'
raw=table.read_bytes()
for pos in range(0,len(raw)-31,32):
    magic,kind,subtype,offset,size,label,flags=struct.unpack_from('<HBBII16sI',raw,pos)
    if magic!=0x50aa:break
    parts[label.split(b'\0')[0].decode()]=(offset,size)
assert parts=={'nvs':(0x9000,0x6000),'phy_init':(0xf000,0x1000),
    'factory':(0x10000,0x800000),'spiffs':(0x810000,0x360000),'roms':(0xb70000,0x400000)}
previous_end=0
for offset,size in sorted(parts.values()):
    assert offset>=previous_end and offset+size<=0x1000000
    previous_end=offset+size
app=build/'firmware.bin'
assert 0<app.stat().st_size<=parts['factory'][1]
assert app.read_bytes()[0]==0xe9
assets=build/'littlefs.bin'
assert assets.stat().st_size==parts['spiffs'][1]
romdir=root/'integration/chessengine/build-host/rom-image'
rom=romdir/'roms-fat-wl.bin'
manifest=json.loads((romdir/'manifest.json').read_text())
assert rom.stat().st_size==manifest['size']==parts['roms'][1]
assert hashlib.sha256(rom.read_bytes()).hexdigest()==manifest['sha256']
# The FAT/WL bytes are partition-relative; its new offset is taken from the
# combined partition table rather than changing the standalone ROM manifest.
segments=[(0,build/'bootloader.bin'),(0x8000,table),
    (parts['factory'][0],app),(parts['spiffs'][0],assets),(parts['roms'][0],rom)]
for offset,path in segments:
    print(hex(offset),path.stat().st_size,path.name,hashlib.sha256(path.read_bytes()).hexdigest(),flush=True)
if a.write:
    device=next((d for d in list_ports.comports() if d.device==a.port),None)
    assert device and device.vid==0x303a and device.pid==0x1001 and (device.serial_number or '').replace(':','').upper()=='44BD8D60CB44'
    command=[sys.executable,'-m','esptool','--chip','esp32s3','--port',a.port,
        '--baud','921600','write-flash','--flash-mode','dio','--flash-freq','80m','--flash-size','16MB']
    for offset,path in segments:command.extend([hex(offset),str(path)])
    subprocess.run(command,check=True,env={**os.environ,'PYTHONIOENCODING':'utf-8','PYTHONUTF8':'1'})
else:print('Combined application validated; no flash writes requested.')
