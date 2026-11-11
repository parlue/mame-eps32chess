"""Observe real finger events without injecting taps or resetting the board."""
import argparse
import time
from pathlib import Path
import serial
from serial.tools import list_ports
p=argparse.ArgumentParser()
p.add_argument('--port',default='COM8')
p.add_argument('--seconds',type=float,default=180)
p.add_argument('--log',type=Path,required=True)
a=p.parse_args()
d=next((x for x in list_ports.comports() if x.device==a.port),None)
assert d and d.vid==0x303a and d.pid==0x1001 and (d.serial_number or '').replace(':','').upper()=='44BD8D60CB44'
usb=serial.Serial(None,115200,timeout=.2);usb.dtr=False;usb.rts=False;usb.port=a.port
with usb,a.log.open('w',encoding='utf-8') as log:
    end=time.monotonic()+a.seconds
    print('Monitoring real touch events; no injected input',flush=True)
    while time.monotonic()<end:
        line=usb.readline().decode(errors='replace').strip()
        if line:log.write(line+'\n');log.flush();print(line,flush=True)
