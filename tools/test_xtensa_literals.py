"""Check actual linked string addresses for the reproduced Xtensa ld bug."""
from pathlib import Path
import struct
import subprocess

root = Path(__file__).resolve().parents[1]
tool = Path.home() / '.platformio/packages/toolchain-xtensa-esp-elf/bin'
out = root / 'build-host/literal-probe'
out.mkdir(parents=True, exist_ok=True)

def run(name, *args):
    subprocess.run([str(tool / ('xtensa-esp32s3-elf-' + name + '.exe')), *map(str, args)], check=True)

flags = ['-Os', '-fno-jump-tables', '-ffunction-sections', '-fdata-sections', '-mtext-section-literals', '-mlongcalls']
anchor = out / 'anchor.o'
run('g++', *flags, '-c', root / 'tools/xtensa_literal_anchor.cpp', '-o', anchor)

def linked_ctrlr_references(path):
    data = path.read_bytes()
    assert data[:6] == b'\x7fELF\x01\x01'
    section_offset = struct.unpack_from('<I', data, 32)[0]
    size, count, names_index = struct.unpack_from('<HHH', data, 46)
    headers = [struct.unpack_from('<10I', data, section_offset + i * size) for i in range(count)]
    name_header = headers[names_index]
    names = data[name_header[4]:name_header[4] + name_header[5]]
    sections = {}
    for h in headers:
        name = names[h[0]:].split(b'\0', 1)[0].decode()
        sections[name] = (h[3], data[h[4]:h[4] + h[5]])
    address, rodata = sections['.rodata']
    text = sections['.text'][1]
    refs = 0
    offset = 0
    while True:
        offset = rodata.find(b'ctrlr\0', offset)
        if offset < 0:
            break
        refs += text.count(struct.pack('<I', address + offset))
        offset += 6
    return refs

results = {}
for variant, extra in [('default', []), ('no-merge', ['-fno-merge-constants'])]:
    obj = out / (variant + '.o')
    elf = out / (variant + '.elf')
    run('g++', *flags, *extra, '-c', root / 'tools/xtensa_literal_probe.cpp', '-o', obj)
    run('ld', '-e', 'probe', '-Ttext=0x42000000', anchor, obj, '-o', elf)
    results[variant] = linked_ctrlr_references(elf)
    print(variant + ': ' + str(results[variant]) + ' correct ctrlr references (expected 2)', flush=True)
if results['no-merge'] != 2:
    raise SystemExit('FAIL: targeted MAME compiler correction did not preserve both references')
print('PASS: targeted no-merge fix; default linker bug reproduced' if results['default'] == 1 else 'PASS: targeted fix; default toolchain no longer reproduces the original bug')
