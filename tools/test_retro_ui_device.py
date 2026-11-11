"""Drive the real UI event path and capture scanlines also sent to the LCD.

USB tap injection tests controller routing, not the physical touch transform.
Touch controller detection is checked; Dirk confirms actual finger operation.
"""
import argparse
import json
import re
import struct
import time
import zlib
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',default='COM8')
p.add_argument('--output',type=Path,required=True)
p.add_argument('--boot-only',action='store_true')
a=p.parse_args()
d=next((x for x in list_ports.comports() if x.device==a.port),None)
assert d and d.vid==0x303a and d.pid==0x1001 and (d.serial_number or '').replace(':','').upper()=='44BD8D60CB44','Wrong Waveshare device'
a.output.mkdir(parents=True,exist_ok=True)

def png(path,raw):
    assert len(raw)==460800
    rows=bytearray()
    for y in range(480):
        rows.append(0)
        for (pixel,) in struct.iter_unpack('>H',raw[y*960:(y+1)*960]):
            rows.extend((((pixel>>11)&31)*255//31,((pixel>>5)&63)*255//63,(pixel&31)*255//31))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',480,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))

with serial.Serial(a.port,115200,timeout=.1) as usb,(a.output/'device.log').open('w',encoding='utf-8') as log:
    buffer=bytearray();lines=[];captures=[];replies=[]
    def record(line):
        lines.append(line);log.write(line+'\n');log.flush();print(line,flush=True)
    def send(line):record('[TEST SEND] '+line);usb.write((line+'\n').encode())
    def wait(pattern,seconds=180):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if b'\n' not in buffer:buffer.extend(usb.read(max(1,usb.in_waiting)))
            if b'\n' not in buffer:continue
            raw,_,rest=buffer.partition(b'\n');buffer[:]=rest
            line=raw.decode(errors='replace').strip()
            if not line:continue
            record(line)
            assert not any(x in line for x in ['Guru Meditation','abort() was called','[ALLOC FAIL]','[UCI ERROR]','[LCD ERROR]']),line
            if 'Maschine beendet, Ergebnis' in line:assert line.endswith('Ergebnis 0'),line
            if re.search(pattern,line):return line
        raise AssertionError('Timeout waiting for '+pattern)
    def tap(x,y):send(f'tap {x} {y}');wait(r'^\[UI TAP\] '+str(x)+' '+str(y),30)
    def capture(name):
        send('screenshot');wait(r'^\[LCD FRAME\] 480 480 RGB565BE 460800$',30)
        end=time.monotonic()+60
        while len(buffer)<460800 and time.monotonic()<end:buffer.extend(usb.read(max(1,min(16384,460800-len(buffer)))))
        assert len(buffer)>=460800,'Incomplete LCD stream'
        raw=bytes(buffer[:460800]);del buffer[:460800]
        png(a.output/(name+'.png'),raw);wait(r'^\[LCD FRAME END\]',30);captures.append(name+'.png')
    usb.dtr=False;usb.rts=True;time.sleep(.1);usb.rts=False
    wait(r'^\[LOAD READY\]',40)
    assert any('[LCD READY]' in s for s in lines),'LCD not initialized'
    assert any('[TOUCH READY]' in s for s in lines),'Touch not detected'
    capture('00-retro-start')
    if a.boot_only:
        record('PASS: reset boot, LCD and touch initialization, binary LCD capture')
        raise SystemExit(0)
    tap(300,340);capture('01-load');tap(240,150)
    wait(r'^\[UI SEND\] uci$');wait(r'^readyok$')
    tap(410,340);capture('02-options');tap(240,105);capture('03-option-editor');tap(240,450)
    tap(60,410);capture('04-colour');tap(100,180)
    wait(r'^\[UI SEND\] position startpos moves e2e4$');wait(r'^bestmove [a-h][1-8][a-h][1-8]')
    replies.append(lines[-1]);capture('05-mm4-both-moves')
    tap(60,340);capture('06-mm4-board');tap(240,450)
    tap(180,340);capture('07-mm4-notation');tap(240,450)
    tap(300,410);capture('08-info');tap(240,450)
    tap(180,410);wait(r'^\[UI SEND\] go$');wait(r'^bestmove [a-h][1-8][a-h][1-8]')
    replies.append(lines[-1]);capture('09-mm4-move-switch')
    tap(410,410);wait(r'^\[LOAD READY\]')
    tap(300,340);tap(240,250);wait(r'^\[UI SEND\] uci$');wait(r'^readyok$')
    tap(60,410);tap(350,180);wait(r'^\[UI SEND\] go$');wait(r'^bestmove [a-h][1-8][a-h][1-8]')
    replies.append(lines[-1]);capture('10-mm5-white-opening')
    tap(180,410);wait(r'^\[UI SEND\] go$');tap(410,410);wait(r'^\[LOAD READY\]')
    capture('11-retro-idle')
    record('PASS: real LCD scanlines, touch controller detected, UI Load/Options/Play/Move/Board/Notation/Info/Quit, MM IV and MM V ROM moves and clean exits')
    (a.output/'evidence.json').write_text(json.dumps({'captures':captures,'actual_rom_replies':replies,
        'capture':'ESP scanline stream also sent to CO5300; not panel readback/camera',
        'input':'USB tap injection into the same UI event path; physical finger test pending'},indent=2),encoding='utf-8')
