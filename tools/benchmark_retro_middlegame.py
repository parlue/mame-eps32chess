"""Measure sustained middlegame emulation on the guarded Waveshare USB device.

Replay time is excluded. ROM level uses emulated time; --seconds is wall time.
No go-movetime assumption: the original plugin ignores that UCI parameter.
"""
import argparse,json,re,statistics,subprocess,time
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--driver',choices=['mm4','mm5'],required=True)
p.add_argument('--case',choices=['spanish-centre','dragon-attack'],required=True)
p.add_argument('--seconds',type=float,default=180)
p.add_argument('--level',default='le 9')
p.add_argument('--port',default='COM8')
p.add_argument('--output',type=Path,required=True)
p.add_argument('--expect-opcode-profile',action='store_true',help='Require diagnostic histograms; resulting speed is instrumented and not comparable')
p.add_argument('--expect-core-profile',action='store_true',help='Require diagnostic nested core scopes; resulting speed is not a candidate speed measurement')
p.add_argument('--expect-quiet-dac',action='store_true',help='Require 11025-Hz discarded DAC audio and the original MM IV/V CPU clock/update period')
p.add_argument('--expect-hot-metadata',action='store_true',help='Require the bounded internal metadata experiment at ROM startup')
p.add_argument('--expect-flat-pages',action='store_true',help='Require direct page pointers, bank invalidation self-test and no later invalidation')
p.add_argument('--expect-flat-postload',action='store_true',help='Also require the post-load invalidation registration')
a=p.parse_args()
assert not (a.expect_opcode_profile and a.expect_core_profile), 'Use one diagnostic at a time'
assert not a.expect_flat_postload or a.expect_flat_pages, 'Post-load check requires direct pages'
if a.seconds<60:p.error('Use at least 60 wall seconds')
if not re.fullmatch(r'(le [0-9]|bl [1-8])',a.level):p.error('Invalid device level')
root=Path(__file__).resolve().parents[1]
case=json.loads((root/'tools/retro_benchmark_positions.json').read_text())[a.case]
checker=root/'integration/chessengine/build-host/position-check/retro_position_check.exe'
position=json.loads(subprocess.run([str(checker)],input=case['moves'],text=True,capture_output=True,check=True).stdout)
device=next((d for d in list_ports.comports() if d.device==a.port),None)
assert device and device.vid==0x303a and device.pid==0x1001 and (device.serial_number or '').replace(':','').upper()=='44BD8D60CB44'
a.output.mkdir(parents=True,exist_ok=False)
events=[];buffer=bytearray();started=time.monotonic();loaded=False
with serial.Serial(a.port,115200,timeout=.2) as usb,(a.output/'device.log').open('w',encoding='utf-8') as log:
    def record(line):
        events.append((time.monotonic(),line));log.write(line+'\n');log.flush();print(line,flush=True)
    def send(line):
        record('[TEST SEND] '+line)
        payload=(line+'\n').encode()
        # Respect the board's small USB RX ring, especially in overflow tests.
        for offset in range(0,len(payload),32):
            chunk=payload[offset:offset+32];assert usb.write(chunk)==len(chunk)
            time.sleep(.05)
    def receive():
        try:return usb.read(min(16384,max(1,usb.in_waiting)))
        except (serial.SerialException,OSError):
            usb.close();deadline=time.monotonic()+15
            while time.monotonic()<deadline:
                try:usb.dtr=False;usb.rts=False;usb.open();record('[USB] reconnected');return b''
                except (serial.SerialException,OSError):time.sleep(.2)
            raise
    def line(timeout=1):
        deadline=time.monotonic()+timeout
        while b'\n' not in buffer and time.monotonic()<deadline:
            buffer.extend(receive());assert len(buffer)<16384,'Unbounded device log line'
        if b'\n' not in buffer:return None
        end=buffer.index(b'\n');result=bytes(buffer[:end]).decode(errors='replace').strip();del buffer[:end+1]
        record(result)
        assert not any(s in result for s in ['Guru Meditation','abort() was called','Stack canary','integrity=FAILED','[UCI ERROR]','[ALLOC FAIL]','[RETRO UCI] info string error']),result
        return result
    def wait(pattern,timeout=90):
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            result=line(min(1,deadline-time.monotonic()))
            if result and re.search(pattern,result):return result
        raise AssertionError('Timeout waiting for '+pattern)
    def tap(x,y):send(f'tap {x} {y}');wait(r'^\[DISPLAY TAP\] '+str(x)+' '+str(y))
    def wait_since(pattern,index,timeout=90):
        for _,result in events[index:]:
            if re.search(pattern,result):return result
        return wait(pattern,timeout)
    def uci(command):
        index=len(events)
        send('retro-uci '+command)
        # Firmware tasks can interleave whole markers with a profile line.
        # Match the complete acknowledgement token, not the line boundaries.
        ack=wait_since(r'\[RETRO DIAGNOSTIC\] (accepted|rejected)\b',index)
        assert re.search(r'\[RETRO DIAGNOSTIC\] (accepted|rejected)\b',ack)[1]=='accepted','USB command rejected'
        return index
    try:
        usb.reset_input_buffer();usb.dtr=False;usb.rts=True;time.sleep(.1);usb.rts=False
        wait(r'^ESP-ROM:esp32s3-',60);wait(r'^\[START\] setup done',60)
        assert any('[START] Bauerino display=1' in s for _,s in events)
        assert any('[START] AP=1' in s for _,s in events)
        time.sleep(4) # Let full-frame startup animation finish before RX stress.
        send('retro-uci go');wait(r'\[RETRO DIAGNOSTIC\] rejected\b')
        send('x'*1030+'tap 100 320');wait(r'^\[DIAGNOSTIC\] rejected: input line too long$')
        tap(100,320);tap(300,340);tap(240,150 if a.driver=='mm4' else 250)
        loaded=True;wait(r'^\[RETRO OPEN\] '+a.driver);wait(r'^\[RETRO UCI\] readyok$')
        metadata = [s for _,s in events if '[MAME HOT METADATA]' in s]
        assert bool(metadata) == a.expect_hot_metadata, 'Wrong metadata placement profile'
        if a.expect_hot_metadata:
            assert len(metadata) == 2 and all(re.search(r'pages=8 bytes=200 internal=1', s) for s in metadata), 'Metadata self-test/rebuild not verified'
        flat = [(i,s) for i,(_,s) in enumerate(events) if '[MAME FLAT PAGES]' in s]
        assert bool(flat) == a.expect_flat_pages, 'Wrong direct-page profile'
        if a.expect_flat_pages:
            assert len(flat) == 3 and all('pointer_bytes=4 bank_changes=fallback' in s for _,s in flat), 'Direct-page helper lifecycle not verified'
            assert any('[MAME FLAT BANK TEST] set_base=OK invalidation=OK fallback=OK' in s for _,s in events), 'Missing bank invalidation test'
            restored = any('[MAME FLAT RESTORE] postload_registered=1' in s for _,s in events)
            assert restored == a.expect_flat_postload, 'Wrong post-load invalidation profile'
        if a.expect_quiet_dac:
            startup = '\n'.join(s for _,s in events)
            rate = re.search(r'\[MAME QUIET DAC\] original=192000 requested=11025 cpu_clock=(\d+) timer_attoseconds=(\d+)', startup)
            before = re.search(r'\[MAME PRE-DEVICE\] sample_rate=11025 update_attoseconds=(\d+)', startup)
            assert rate and before and int(rate[1]) == 4915200 and rate[2] == before[1], 'Quiet DAC changed/missed the original CPU clock or sound-update period'
        uci('setoption name Speed value 0');uci('setoption name Level value '+a.level)
        uci('ucinewgame');ready_index=uci('isready');wait_since(r'^\[RETRO UCI\] readyok$',ready_index,180)
        record('[BENCH] replay_begin plies='+str(position['plies']))
        replay_index=uci('position startpos moves '+case['moves']);ready_index=uci('isready')
        # ESP Runtime intentionally suppresses popmessage/Log. The isready
        # response is serialized after play_moves and its final ENT wait.
        wait_since(r'^\[RETRO UCI\] readyok$',ready_index,900)
        assert not any(s.startswith('[RETRO UCI] bestmove ') for _,s in events[replay_index:]),'ROM finished before measurement'
        record('[BENCH] replay_complete fen='+position['fen'])
        # Multi-move replay already starts the ROM through its ENT key. go is
        # idempotent here and marks the visible frontend as thinking as well.
        go_index=uci('go');wait_since(r'^\[UI SEND\] go$',go_index)
        search=time.monotonic();record('[BENCH] search_begin wall_seconds='+str(a.seconds))
        reply=None
        while time.monotonic()-search<a.seconds:
            result=line()
            if result and result.startswith('[RETRO UCI] bestmove '):
                reply=result.split()[3];assert reply in position['legal'],'Illegal ROM reply';break
        end=time.monotonic();record('[BENCH] search_end')
        windows=[];cpu=[];profiles=[]
        for timestamp,result in events:
            # Only intervals ending >=10s after go; exclude replay overlap.
            if not search+10<=timestamp<=end:continue
            match=re.search(r'^\[MAME RATE\] emu=([\d.]+)s real=([\d.]+)s speed=([\d.]+)% heap=(\d+) psram=(\d+)',result)
            if match:windows.append({'emu':float(match[1]),'real':float(match[2]),'speed':float(match[3]),
                                     'internal_free':int(match[4]),'psram_free':int(match[5])})
            match=re.search(r'^\[MAME CPU\].*?wall=([\d.]+)%.*?stack_free=(\d+)',result)
            if match:cpu.append({'wall_percent':float(match[1]),'stack_free':int(match[2])})
            if result.startswith('[MAME PROFILE]'):profiles.append(result)
        assert len(windows)>=8,'Search ended too early for a sustained measurement'
        has_opcode_profile=any('[MAME OPCODE TOTAL]' in s for t,s in events if search+10<=t<=end)
        assert has_opcode_profile==a.expect_opcode_profile, 'Opcode instrumentation does not match requested measurement kind'
        has_core_profile=any('[MAME CORE]' in s for t,s in events if search+10<=t<=end)
        assert has_core_profile==a.expect_core_profile, 'Core instrumentation does not match requested measurement kind'
        if a.expect_flat_pages:
            assert not any('[MAME FAST MEMORY] invalidated' in s for _,s in events[flat[-1][0]+1:]), 'Direct pages invalidated during ROM replay/search'
        quit_index=uci('quit');wait_since(r'^\[RETRO\] exit 0; integrity=OK',quit_index,90);loaded=False
        tap(30,25)
        elapsed=sum(w['real'] for w in windows);emulated=sum(w['emu'] for w in windows)
        evidence={'driver':a.driver,'case':a.case,'level':a.level,'position':position,
            'measurement_kind':'instrumented-core-profile' if a.expect_core_profile else ('instrumented-opcode-profile' if a.expect_opcode_profile else 'sustained-speed'),
            'quiet_dac_contract_verified':a.expect_quiet_dac,
            'hot_metadata_verified':a.expect_hot_metadata,
            'flat_pages_verified':a.expect_flat_pages,
            'flat_postload_verified':a.expect_flat_postload,
            'search_wall_seconds':round(end-search,3),'bestmove':reply,'search_windows':windows,
            'measured_wall_seconds':elapsed,'measured_emulated_seconds':emulated,
            'weighted_original_speed_percent':100*emulated/elapsed,
            'median_window_percent':statistics.median(w['speed'] for w in windows),
            'cpu_windows':cpu,'profiles':profiles,'clean_exit':True,
            'limits':'Requested search phase after replay; no internal ROM RAM/depth readback. CPU wall time includes preemption. Not physical phone/touch acceptance.'}
        (a.output/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
        record('PASS: sustained middlegame '+a.driver+' '+a.case+' speed='+str(round(100*emulated/elapsed,2))+'%')
    finally:
        if loaded:
            try:
                # Home uses the independent priority stop request even if the
                # diagnostic queue/session is not ready after an error.
                send('tap 30 25');wait(r'^\[RETRO\] exit 0; integrity=OK',90)
            except Exception as cleanup_error:record('[CLEANUP FAILED] '+str(cleanup_error))
        (a.output/'events.json').write_text(json.dumps([(round(t-started,3),s) for t,s in events],indent=2)+'\n')
