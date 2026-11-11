"""Build a private FAT/WL image for the existing 4 MiB ROM partition.
Uses ESP-IDF's own generator, without changing the active firmware build.
"""
from pathlib import Path
import csv,hashlib,json,shutil,subprocess,sys
root=Path(__file__).resolve().parents[1]
partitions=root.parents[1]/'esp32s3/partitions.csv'
rows={row[0].strip():row for row in csv.reader(line for line in partitions.read_text().splitlines() if line.strip() and not line.lstrip().startswith('#'))}
rom_offset=int(rows['roms'][3].strip(),0)
rom_size=int(rows['roms'][4].strip(),0)
if rom_size!=0x400000 or rom_offset+rom_size>0x1000000:raise SystemExit('Unsupported ROM partition layout')
idf=Path.home()/'.platformio/packages/framework-espidf'
out=root/'build-host/rom-image';files=out/'files';files.mkdir(parents=True,exist_ok=True)
manifest=json.loads((root/'local-roms/manifest.json').read_text())
for item in manifest:
    src=root/'local-roms'/item['target'];data=src.read_bytes()
    if len(data)!=item['bytes'] or hashlib.sha256(data).hexdigest()!=item['sha256']:raise SystemExit('ROM hash changed: '+str(src))
    target=files/item['target'];target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,target)
for directory in ['cfg','nvram','sta','ini']:(files/directory).mkdir(exist_ok=True)
image=out/'roms-fat-wl.bin'
subprocess.run([sys.executable,str(idf/'components/fatfs/wl_fatfsgen.py'),str(files),'--output_file',str(image),'--partition_size','4194304','--sector_size','4096','--long_name_support','--use_default_datetime'],check=True)
if image.stat().st_size!=0x400000:raise SystemExit('Wrong ROM partition image size')
record={'offset':hex(rom_offset),'size':image.stat().st_size,'sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'roms':manifest,'format':'ESP-IDF FAT + wear levelling; 4096 byte sectors; long names'}
(out/'manifest.json').write_text(json.dumps(record,indent=2),encoding='utf-8')
print('Prepared private ROM filesystem:',image,'SHA256',record['sha256'])
