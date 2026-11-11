"""Exercise the combined frontend and actual ROM task over guarded USB."""
import argparse,json,re,struct,time,zlib
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',default='COM8')
p.add_argument('--output',type=Path,required=True)
p.add_argument('--boot-only',action='store_true')
p.add_argument('--skip-intro-capture',action='store_true')
p.add_argument('--no-captures',action='store_true')
p.add_argument('--cache-mode',type=int,choices=[0,1],help='Select 6502 data access before loading a ROM')
p.add_argument('--speed-mode',type=int,choices=[0,1,2,3],help='Bit 0: cached data; bit 1: internal Mephisto CPU')
p.add_argument('--http-during-rom',action='store_true',help='Exercise portal HTTP while the MM V worker is alive')
a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
device=next((d for d in list_ports.comports() if d.device==a.port),None)
assert device and device.vid==0x303a and device.pid==0x1001 and (device.serial_number or '').replace(':','').upper()=='44BD8D60CB44'

def png(path,raw):
    rgb=bytearray()
    for (pixel,) in struct.iter_unpack('>H',raw):
        rgb.extend((((pixel>>11)&31)*255//31,((pixel>>5)&63)*255//63,(pixel&31)*255//31))
    data=b''.join(b'\0'+rgb[y*1440:(y+1)*1440] for y in range(480))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',480,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(data))+chunk(b'IEND',b''))

usb=serial.Serial(a.port,115200,timeout=.2)
buffer=bytearray();lines=[];captures=[]
with usb,(a.output/'device.log').open('w',encoding='utf-8') as log:
    def record(line):
        lines.append(line);log.write(line+'\n');log.flush();print(line,flush=True)
    def send(line):record('[TEST SEND] '+line);usb.write((line+'\n').encode())
    def receive(maximum=16384,reconnect=True):
        try:return usb.read(min(maximum,max(1,usb.in_waiting)))
        except (serial.SerialException,OSError):
            if not reconnect:raise
            usb.close();deadline=time.monotonic()+15
            while time.monotonic()<deadline:
                target=next((d for d in list_ports.comports() if d.vid==0x303a and d.pid==0x1001 and (d.serial_number or '').replace(':','').upper()=='44BD8D60CB44'),None)
                if target:
                    try:
                        usb.port=target.device;usb.dtr=False;usb.rts=False;usb.open()
                        record('[USB] reconnected after reset');return b''
                    except (serial.SerialException,OSError):pass
                time.sleep(.2)
            raise AssertionError('USB device did not return after reset')
    def wait(pattern,seconds=180):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            while b'\n' not in buffer and time.monotonic()<end:buffer.extend(receive())
            if b'\n' not in buffer:break
            endline=buffer.index(b'\n');raw=bytes(buffer[:endline]);del buffer[:endline+1]
            line=raw.decode(errors='replace').strip()
            if not line:continue
            record(line)
            assert not any(x in line for x in ('Guru Meditation','abort() was called','Stack canary','integrity=FAILED','[UCI ERROR]','[ALLOC FAIL]','[RETRO UCI] info string error')),line
            if re.search(pattern,line):return line
        raise AssertionError('Timeout waiting for '+pattern)
    def tap(x,y):send(f'tap {x} {y}');wait(r'^\[DISPLAY TAP\] '+str(x)+' '+str(y),30)
    def capture(name):
        if a.no_captures:record('[CAPTURE SKIPPED] '+name);return
        send('screenshot');wait(r'^\[LCD FRAME\] 480 480 RGB565BE 460800$',30)
        end=time.monotonic()+60
        while len(buffer)<460800 and time.monotonic()<end:buffer.extend(receive(min(16384,460800-len(buffer)),False))
        assert len(buffer)>=460800,'Incomplete frontend capture'
        raw=bytes(buffer[:460800]);del buffer[:460800]
        png(a.output/(name+'.png'),raw);wait(r'^\[LCD FRAME END\]',30);captures.append(name+'.png')
    usb.reset_input_buffer()
    usb.dtr=False;usb.rts=True;time.sleep(.1);usb.rts=False
    # Ignore a previous boot's partial tail when attaching just after a flash.
    wait(r'^ESP-ROM:esp32s3-',60)
    wait(r'^\[START\] setup done',60)
    assert any('[START] Bauerino display=1' in s for s in lines),'Display init failed'
    assert any('[START] AP=1' in s for s in lines),'Wi-Fi AP failed'
    if a.cache_mode is not None:
        send('mame-cache '+str(a.cache_mode));wait(r'^\[MAME CACHE\] enabled='+str(a.cache_mode),30)
    if a.speed_mode is not None:
        send('mame-speed '+str(a.speed_mode));wait(r'^\[MAME SPEED\] mode='+str(a.speed_mode),30)
    if a.skip_intro_capture:time.sleep(20)
    else:capture('00-start-animation');time.sleep(3.5)
    capture('01-app-menu')
    if a.boot_only:
        record('PASS: combined startup, display/touch/assets and Wi-Fi AP initialization');raise SystemExit(0)
    tap(100,140);capture('02-virginia-setup')
    tap(100,455);capture('03-virginia-board')
    tap(230,455);capture('04-virginia-notation');tap(375,455)
    tap(300,140);capture('05-bot-groups');tap(100,120);capture('06-bot-elo')
    tap(90,150);capture('07-bot-personalities');tap(90,150);capture('08-bot-colour')
    tap(100,415);capture('09-bot-preview');tap(375,455)
    send('bots-test');wait(r'^\[BOTS TEST\] decoded=215/215',60)
    send('swipe -180');wait(r'^\[DISPLAY\] home page=2',30);capture('10-setup-page')
    send('swipe 180');wait(r'^\[DISPLAY\] home page=1',30)
    tap(100,320);capture('11-retro');tap(300,340);capture('12-retro-load');tap(240,150)
    wait(r'^\[RETRO OPEN\] mm4');wait(r'^\[RETRO UCI\] readyok$')
    tap(410,340);capture('13-retro-options');tap(240,450)
    tap(60,410);capture('14-retro-colour');tap(100,180)
    move=wait(r'^\[RETRO UCI\] bestmove [a-h][1-8][a-h][1-8]')
    capture('15-retro-both-moves');tap(60,340);capture('16-retro-board');tap(240,450)
    tap(180,340);capture('17-retro-notation');tap(240,450)
    tap(410,410);wait(r'^\[RETRO\] exit 0; integrity=OK')
    tap(300,340);tap(240,250);wait(r'^\[RETRO OPEN\] mm5');wait(r'^\[RETRO UCI\] readyok$')
    tap(60,410);tap(350,180)
    move2=wait(r'^\[RETRO UCI\] bestmove [a-h][1-8][a-h][1-8]');capture('18-mm5-white-opening')
    if a.http_during_rom:
        send('http-test')
        wait(r'^\[HTTP TEST\] /unknown-portal-probe received=\d+ bytes',90)
        for path in ['/', '/api/state', '/generate_204', '/hotspot-detect.html', '/connecttest.txt', '/unknown-portal-probe']:
            assert any('[HTTP TEST] '+path+' -> HTTP/1.1 ' in s and (' 200 ' in s or ' 302 ' in s) for s in lines), 'HTTP failure: '+path
    tap(30,25);wait(r'^\[RETRO\] exit 0; integrity=OK');time.sleep(.3);capture('19-return-home')
    tap(100,140);capture('20-virginia-after-retro')
    record('PASS: original launcher/Virginia/Bots/gestures, real MM IV/MM V ROM/UCI controls, clean task exits and return to original frontend')
    (a.output/'evidence.json').write_text(json.dumps({'captures':captures,'rom_replies':[move,move2],
        'capture':'Framebuffer sent to panel; not panel readback or camera',
        'input':'USB gestures injected into the physical gesture path; final finger verification required'},indent=2),encoding='utf-8')
