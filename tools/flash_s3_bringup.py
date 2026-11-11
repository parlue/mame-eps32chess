"""Validate the standalone image, then optionally flash the backed-up board.
Run with the existing Bauerino Python environment (esptool 5 installed).
"""
import argparse, hashlib, json, os, struct, subprocess, sys
from pathlib import Path
from serial.tools import list_ports

root=Path(__file__).resolve().parents[1]
build=root/'esp32s3/.pio/build/esp32s3-skeleton'
p=argparse.ArgumentParser()
p.add_argument('--write',action='store_true')
p.add_argument('--port',default='COM8')
args=p.parse_args()
backup=root/'backups/takeover-20261110/waveshare-before-mame-16MB.bin'
assert backup.stat().st_size==0x1000000, 'Full flash backup missing'
assert hashlib.sha256(backup.read_bytes()).hexdigest()=='4264eb20d86cfd878d9135853a6b27c1ec817379fa63fbfc15245250c38a8e1e', 'Backup hash mismatch'

table=build/'bringup-partitions.bin'
generator=Path.home()/'.platformio/packages/framework-espidf/components/partition_table/gen_esp32part.py'
subprocess.run([sys.executable,str(generator),'--offset','0x8000','--flash-size','16MB',str(root/'esp32s3/partitions.csv'),str(table)],check=True)
parts={}
data=table.read_bytes()
for pos in range(0,len(data)-31,32):
    magic,kind,subtype,offset,size,label,flags=struct.unpack_from('<HBBII16sI',data,pos)
    if magic!=0x50aa:break
    parts[label.split(b'\0',1)[0].decode()]=(offset,size)
app=build/'firmware.bin'
assert parts['factory'][0]==0x10000
assert 0<app.stat().st_size<=parts['factory'][1], 'Firmware exceeds app partition'
romdir=root/'integration/chessengine/build-host/rom-image'
rom=romdir/'roms-fat-wl.bin'
manifest=json.loads((romdir/'manifest.json').read_text())
assert parts['roms']==(int(manifest['offset'],0),manifest['size']), 'ROM offset/size differs from compiled partition table'
assert hashlib.sha256(rom.read_bytes()).hexdigest()==manifest['sha256'], 'ROM image hash mismatch'
assert parts['factory'][0]+parts['factory'][1]<=parts['roms'][0]
assert parts['roms'][0]+parts['roms'][1]<=0x1000000
boot=build/'bootloader/bootloader.bin'
if not boot.exists():boot=build/'bootloader.bin'
segments=[(0,boot),(0x8000,table),(0x10000,app),(parts['roms'][0],rom)]
for offset,path in segments:print(hex(offset),path.stat().st_size,path.name,flush=True)
if args.write:
    device=next((d for d in list_ports.comports() if d.device==args.port),None)
    assert device and device.vid==0x303a and device.pid==0x1001 and (device.serial_number or '').replace(':','').upper()=='44BD8D60CB44', 'Not the backed-up Waveshare device'
    command=[sys.executable,'-m','esptool','--chip','esp32s3','--port',args.port,'--baud','921600','write-flash','--flash-mode','dio','--flash-freq','80m','--flash-size','16MB']
    for offset,path in segments:command.extend([hex(offset),str(path)])
    subprocess.run(command,check=True,env={**os.environ,'PYTHONIOENCODING':'utf-8','PYTHONUTF8':'1'})
else:print('Validation passed; no flash writes requested.')
