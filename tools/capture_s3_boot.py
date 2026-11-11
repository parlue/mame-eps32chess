"""Capture a reset/boot of the known Waveshare board. No flash writes."""
import argparse, time
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',default='COM8')
p.add_argument('--seconds',type=float,default=90)
p.add_argument('--log',type=Path,required=True)
args=p.parse_args()
device=next((d for d in list_ports.comports() if d.device==args.port),None)
if not device or device.vid!=0x303a or device.pid!=0x1001 or device.serial_number.replace(':','').upper()!='44BD8D60CB44':
    raise SystemExit('Refusing reset: port is not the backed-up Waveshare USB device')
args.log.parent.mkdir(parents=True,exist_ok=True)
with serial.Serial(args.port,115200,timeout=.2) as s, args.log.open('wb') as out:
    # Same hardware-reset sequence as esptool HardReset, after a normal flash.
    s.dtr=False
    s.rts=True
    time.sleep(.1)
    s.rts=False
    until=time.monotonic()+args.seconds
    while time.monotonic()<until:
        data=s.read(max(1,s.in_waiting))
        if data:
            out.write(data);out.flush()
            print(data.decode(errors='replace'),end='',flush=True)
