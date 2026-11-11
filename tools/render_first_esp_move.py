"""Render the real Retro drawing functions with a proven ESP ROM move.

This is a desktop render, not a photograph or a framebuffer capture. Refuses
failed/unconfirmed device logs; original firmware drawing code stays unchanged.
"""
import argparse
import ast
import hashlib
import json
import re
import struct
import subprocess
import zlib
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--project', type=Path, required=True)
p.add_argument('--log', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--retro-base-output', type=Path, help='Export original Retro background as little-endian RGB565 for the real LCD')
a = p.parse_args()
log = a.log.read_text(encoding='utf-8')
if 'PASS: actual ESP ROM + UCI, bestmove ' not in log or 'Maschine beendet, Ergebnis 0' not in log:
    raise SystemExit('Refusing screenshot: no confirmed ESP ROM reply and clean exit')
positions = re.findall(r'^\[SEND\] position startpos moves ([a-h1-8qrbn ]+)$', log, re.M)
replies = re.findall(r'^bestmove ([a-h][1-8][a-h][1-8][qrbn]?)$', log, re.M)
if len(positions) != 1 or len(replies) != 1:
    raise SystemExit('Expected one unambiguous position and one ROM reply')
moves = positions[0].split() + replies
engine_match = re.search(r'^id name (.+)$', log, re.M)
engine_name = engine_match.group(1) if engine_match else 'Mephisto MM IV'
engine_title = engine_name.split(' (')[0]
if any(not re.fullmatch(r'[a-h][1-8][a-h][1-8][qrbn]?', m) for m in moves):
    raise SystemExit('Invalid move token in evidence')

root = a.project.resolve()
out = a.output.resolve()
build = out / 'render'
build.mkdir(parents=True, exist_ok=True)
source = (root / 'src/bauerino_display.cpp').read_text(encoding='utf-8')
gallery = ast.parse((root / 'tools/create_screenshot_gallery.py').read_text(encoding='utf-8'))
prefix = next(ast.literal_eval(n.value) for n in gallery.body
              if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'prefix' for t in n.targets))
draw = source[source.index('void text('):source.index('void tap(')]
prefix += '\nstd::string retroDisplayTitle="bauerino retrochess";\n'
draw = draw.replace('"bauerino retrochess"', 'retroDisplayTitle')
mask_line = "uint8_t mask=c>='0' && c<='9'?digits[c-'0']:0x40;"
assert mask_line in draw
draw = draw.replace(mask_line, "const uint8_t letters[]={0x77,0x7c,0x58,0x5e,0x79,0x71,0x6f,0x74};\n  uint8_t mask=c>='0' && c<='9'?digits[c-'0']:c>='a' && c<='h'?letters[c-'a']:0x40;")
move_literals = ','.join(json.dumps(m) for m in moves)
main = r'''
int main(int argc,char** argv){
  if(argc!=3)return 1;std::string data=argv[1],output=argv[2];
  init_bitboards();init_zobrist();
  wallpaper=(uint16_t*)asset(data+"/wallpaper.rgb");
  pieces=(uint16_t*)asset(data+"/pieces.rgb");
  for(int i=0;i<6;++i)queens[i]=(uint16_t*)asset(data+"/queen-"+std::to_string(i)+".rgb");
  for(int i=0;i<5;++i)cards[i]=(uint8_t*)asset(data+"/app-"+std::to_string(i)+".rgba");
  core2::Game g;g.start(WHITE);g.setup=false;
  for(const char* move:{MOVES}){
    auto legal=find_legal_move(g.pos,move);if(!legal)return 2;g.commit(legal);
  }
  g.message="ROM-Zug: REPLY";
  drawGame(g,BoardView{},true,false,"",SearchView{},true);
  frame.save(output+"/retro-erster-rom-zug.ppm");
  notation=true;drawGame(g,BoardView{},true,false,"",SearchView{},true);
  frame.save(output+"/retro-erste-notation.ppm");
  drawRetro();frame.save(output+"/retro-base.ppm");
  retroDisplayTitle=ENGINE_TITLE;drawRetro();
  center(240,43,"ESP32-S3 | MAME 0.220",0xe6b3,1);
  const char* lcdMoves[]={FIRST_MOVE,LAST_MOVE};
  for(int n=0;n<2;++n){
    int y=72+n*99;
    frame.fillRect(232,y+20,17,47,0x0800);
    for(int i=0;i<4;++i)retroDigit(84+i*76+(i>=2?18:0),y,lcdMoves[n][i]);
    frame.drawFastHLine(228,y+42,18,0xf800);
    frame.fillTriangle(242,y+38,248,y+42,242,y+46,0xf800);
    text(43,y+39,n?"MM IV":"WEISS",0xe6b3,1);
  }
  center(240,277,"1. e4 e5 | Echter ESP-ROM-Test",0xe6b3,1);
  frame.save(output+"/retro-mephisto-beide-zuege.ppm");
}
'''.replace('MOVES', move_literals).replace('REPLY', replies[0]).replace('ENGINE_TITLE',json.dumps(engine_title)).replace('FIRST_MOVE',json.dumps(moves[0])).replace('LAST_MOVE',json.dumps(replies[0]))
(build / 'renderer.cpp').write_text(prefix + draw + main, encoding='utf-8')
font = root / '.pio/libdeps/bauerino480/GFX Library for Arduino/src/font'
if not (font / 'glcdfont.h').exists():
    font = Path(__file__).resolve().parents[2] / 'output/bauerino-prep/.pio/libdeps/bauerino480/GFX Library for Arduino/src/font'
if not (font / 'glcdfont.h').exists():
    raise SystemExit('Original Arduino_GFX font not found')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvars64.bat')
libs = ['types', 'bitboard_compact', 'position', 'movegen', 'weights', 'eval', 'botdata', 'tt', 'search', 'adaptive', 'uci']
objects = [root / 'build/native' / (s + '.obj') for s in libs]
inputs = objects if all(o.exists() for o in objects) else [root / 'lib/chess/src' / (s + '.cpp') for s in libs]
cmd = 'call "' + str(vc) + '" >nul && cl /nologo /std:c++17 /EHsc /utf-8 /O2 /DVIRGIN_EMBEDDED=1 /DVIRGIN_CORE2=1 '
for d in [root / 'include', root / 'lib/chess/src', root / 'tools', font]:
    cmd += '/I"' + str(d) + '" '
cmd += '"' + str(build / 'renderer.cpp') + '" ' + ' '.join('"' + str(s) + '"' for s in inputs)
cmd += ' /Fe:"' + str(build / 'renderer.exe') + '"'
(build / 'compile.cmd').write_text('@echo off\n' + cmd + '\n', encoding='utf-8')
subprocess.run('cmd.exe /d /s /c ""' + str(build / 'compile.cmd') + '""', cwd=build, check=True)
subprocess.run([str(build / 'renderer.exe'), str(root / 'data/bauerino'), str(out)], check=True)
for ppm in out.glob('*.ppm'):
    raw = ppm.read_bytes()
    header = b'P6\n480 480\n255\n'
    assert raw.startswith(header) and len(raw) == len(header) + 480 * 480 * 3
    pixels = raw[len(header):]
    if ppm.stem == 'retro-base' and a.retro_base_output:
        a.retro_base_output.parent.mkdir(parents=True, exist_ok=True)
        rgb565 = bytearray()
        for offset in range(0, len(pixels), 3):
            r, g, b = pixels[offset:offset + 3]
            rgb565.extend(struct.pack('<H', ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
        a.retro_base_output.write_bytes(rgb565)
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + pixels[y * 1440:(y + 1) * 1440] for y in range(480))
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 480, 480, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')
    ppm.with_suffix('.png').write_bytes(png)
    ppm.unlink()
manifest = {
    'description': 'Desktop rendering with actual verified ESP ROM moves; not a device capture. Includes the requested Retro LCD presentation with Mephisto title and both moves; alphabetic segments added for this rendering.',
    'engine': engine_name,
    'moves': moves,
    'log_sha256': hashlib.sha256(a.log.read_bytes()).hexdigest(),
    'drawing_source_sha256': hashlib.sha256(source.encode()).hexdigest(),
}
(out / 'evidence.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding='utf-8')
print('Rendered actual ESP ROM move:', ' '.join(moves), '->', out)
