"""Exercise actual ROM -> Lua/UCI -> USB on the backed-up Waveshare board."""
import argparse,re,time,struct,zlib
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',default='COM8')
p.add_argument('--seconds',type=float,default=300)
p.add_argument('--driver',choices=['mm4','mm5'],default='mm4')
p.add_argument('--sequence',nargs='+',choices=['mm4','mm5'],help='Load/quit several ROMs without resetting the ESP between them')
p.add_argument('--first-move',default='e2e4',choices=[f+'2'+f+r for f in 'abcdefgh' for r in '34']+['b1a3','b1c3','g1f3','g1h3'])
p.add_argument('--games-per-load',type=int,default=1,help='Exercise ucinewgame repeatedly before quitting each loaded ROM')
p.add_argument('--heap-snapshots',action='store_true',help='Record small internal allocations before each load for leak diagnosis')
p.add_argument('--max-idle-loss',type=int,default=64,help='Maximum allowed idle memory loss after warming both drivers (bytes)')
p.add_argument('--benchmark-seconds',type=float,default=0,
               help='Use level le 9 and quit after this many wall seconds of search')
p.add_argument('--log',type=Path,required=True)
p.add_argument('--screenshot',type=Path,help='Capture the scanlines actually repainted on the physical CO5300 after bestmove')
a=p.parse_args()
if a.games_per_load<1:p.error('--games-per-load must be positive')
if a.max_idle_loss<0:p.error('--max-idle-loss must not be negative')
if a.games_per_load!=1 and (a.benchmark_seconds or a.screenshot):p.error('Repeated games require normal move tests')
if a.screenshot and (a.benchmark_seconds or a.sequence):p.error('--screenshot requires one normal ROM move test')

def save_lcd_png(path,raw):
    if len(raw)!=460800:raise SystemExit('FAIL: incomplete physical LCD capture')
    rgb=bytearray()
    for (pixel,) in struct.iter_unpack('>H',raw):
        rgb.extend((((pixel>>11)&31)*255//31,((pixel>>5)&63)*255//63,(pixel&31)*255//31))
    rows=b''.join(b'\0'+rgb[y*1440:(y+1)*1440] for y in range(480))
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',480,480,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(rows))+chunk(b'IEND',b''))
d=next((d for d in list_ports.comports() if d.device==a.port),None)
if not d or d.vid!=0x303a or d.pid!=0x1001 or (d.serial_number or '').replace(':','').upper()!='44BD8D60CB44':
    raise SystemExit('Not the backed-up Waveshare USB device')
a.log.parent.mkdir(parents=True,exist_ok=True)
drivers=a.sequence or [a.driver]
round_index=0
game_index=0
idle_memory=[]
stage=0;ready=0;bestmove=None;success=False;search_started=None
with serial.Serial(a.port,115200,timeout=.2) as s,a.log.open('w',encoding='utf-8') as log:
    def record(line):
        log.write(line+'\n');log.flush()
        print(line,flush=True)
    def send(*lines):
        for line in lines:
            record('[SEND] '+line)
            s.write((line+'\n').encode())
    s.dtr=False;s.rts=True;time.sleep(.1);s.rts=False
    end=time.monotonic()+a.seconds
    buffer=b''
    while time.monotonic()<end:
        buffer+=s.read(max(1,s.in_waiting))
        while b'\n' in buffer:
            raw,buffer=buffer.split(b'\n',1)
            line=raw.decode(errors='replace').strip()
            if not line:continue
            record(line)
            if 'Guru Meditation' in line or 'abort() was called' in line or '[UCI ERROR]' in line:
                raise SystemExit('FAIL: device panic/UCI error; see log')
            if 'Maschine beendet, Ergebnis ' in line and not line.endswith('Ergebnis 0'):
                raise SystemExit('FAIL: MAME start/run error; see log')
            if stage==0 and '[LOAD READY]' in line:
                if a.heap_snapshots:send('heap')
                send('load '+drivers[round_index])
            elif stage==0 and '[UCI READY]' in line:
                if not line.startswith('[UCI READY] '+drivers[round_index]+';'):
                    raise SystemExit('FAIL: wrong driver loaded: '+line)
                stage=1;send('uci')
            elif stage==1 and line=='uciok':
                stage=2;send('isready')
            elif stage==2 and line=='readyok':
                ready+=1
                if ready==1:
                    level='le 9' if a.benchmark_seconds else 'le 1'
                    send('setoption name Level value '+level,'setoption name Speed value 0','ucinewgame','isready')
                elif ready==2:
                    search_started=time.monotonic()
                    stage=3;send('position startpos moves '+a.first_move,'go')
            elif stage==3 and re.fullmatch(r'bestmove [a-h][1-8][a-h][1-8][qrbn]?',line):
                legal={f+'7'+f+r for f in 'abcdefgh' for r in '65'}|{'b8a6','b8c6','g8f6','g8h6'}
                if line.split()[1] not in legal:
                    raise SystemExit('FAIL: illegal black reply to initial '+a.first_move+': '+line)
                bestmove=line
                game_index+=1
                record('[GAME PASS] '+drivers[round_index]+' game '+str(game_index)+': '+line)
                if game_index<a.games_per_load:
                    stage=2;ready=1
                    send('ucinewgame','isready')
                    continue
                if a.screenshot:stage=5;send('screenshot')
                else:stage=4;send('quit')
            elif stage==5 and line=='[LCD FRAME] 480 480 RGB565BE 460800':
                payload=bytearray(buffer);buffer=b''
                capture_end=time.monotonic()+60
                while len(payload)<460800 and time.monotonic()<capture_end:
                    payload+=s.read(max(1,min(16384,460800-len(payload))))
                if len(payload)<460800:raise SystemExit('FAIL: timeout receiving physical LCD pixels')
                save_lcd_png(a.screenshot,payload[:460800]);buffer=bytes(payload[460800:])
                record('[LCD CAPTURE SAVED] '+str(a.screenshot))
                stage=4;send('quit')
            elif stage==4 and 'Maschine beendet, Ergebnis 0' in line:
                stage=6
            elif stage==6 and line.startswith('[MAME IDLE]'):
                snapshot=re.fullmatch(r'\[MAME IDLE\] psram=(\d+) internal=(\d+) integrity=OK',line)
                if not snapshot:raise SystemExit('FAIL: heap integrity check failed')
                idle_memory.append(tuple(map(int,snapshot.groups())))
                # Allow one-time caches for each driver; further cycles must
                # not retain substantial memory in either allocator region.
                if len(idle_memory)>2:
                    baseline=idle_memory[1]
                    if any(now<start-a.max_idle_loss for now,start in zip(idle_memory[-1],baseline)):
                        raise SystemExit('FAIL: retained memory exceeds allowed idle loss after warm-up')
                record('PASS: ROM round '+str(round_index+1)+' '+drivers[round_index]+', '+str(bestmove)+', clean machine exit')
                round_index+=1
                if round_index==len(drivers):success=True;break
                stage=0;ready=0;bestmove=None;search_started=None;game_index=0
        if stage==3 and a.benchmark_seconds and time.monotonic()-search_started>=a.benchmark_seconds:
            record('[BENCHMARK] quit after %.2f wall seconds since go'%(time.monotonic()-search_started))
            bestmove='bounded search benchmark';stage=4;send('quit')
        if success:break
    if success:record('PASS: actual ESP ROM + UCI, '+bestmove+', clean machine exit; drivers='+','.join(drivers))
    else:raise SystemExit(f'FAIL: timeout in stage {stage}; bestmove={bestmove}; see log')
