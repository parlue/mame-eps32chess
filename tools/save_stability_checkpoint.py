"""Save the validated standalone core, evidence and project-owned sources."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
build = root / 'esp32s3/.pio/build/esp32s3-skeleton'
logs = ['takeover-load-cycle-03.log', 'takeover-search-stable-01.log']
for name in logs:
    text = (root / 'docs' / name).read_text(encoding='utf-8')
    assert 'PASS: actual ESP ROM + UCI' in text, f'Incomplete test: {name}'
    assert not any(token in text for token in ['[ALLOC FAIL]', 'Guru Meditation', '[UCI ERROR]', 'integrity=FAILED']), name
    assert all(x == '0' for x in re.findall(r'Maschine beendet, Ergebnis (\d+)', text)), name
    snapshots = re.findall(r'\[MAME IDLE\] psram=(\d+) internal=(\d+) integrity=OK', text)
    assert len(snapshots) >= (6 if name == logs[0] else 2), name
    assert len({psram for psram, _ in snapshots}) == 1, name
    if name == logs[1]:
        searches = re.findall(r'\[BENCHMARK\] quit after ([\d.]+) wall seconds', text)
        assert len(searches) == 2 and all(float(x) >= 120 for x in searches), 'Search regression incomplete'

destination = root / 'backups/takeover-20261110/core-stable-v1'
assert not destination.exists(), 'Refusing to overwrite a checkpoint'
destination.mkdir()
artifacts = ['firmware.bin', 'firmware.elf', 'bootloader.bin', 'bringup-partitions.bin']
for name in artifacts:
    source = build / name
    if name == 'bootloader.bin' and not source.exists():
        source = build / 'bootloader/bootloader.bin'
    shutil.copy2(source, destination / name)
for name in logs + ['takeover-build-22.log', 'takeover-flash-12.log', 'takeover-heap-fixed-diff.txt', '16_stability.md']:
    shutil.copy2(root / 'docs' / name, destination / name)

source_roots = ['esp32s3/src', 'esp32s3/components/mame0220',
                'integration/chessengine/src', 'integration/chessengine/include',
                'integration/chessengine/generated', 'integration/chessengine/lua',
                'integration/chessengine/tools', 'tools']
with zipfile.ZipFile(destination / 'project-sources.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    for relative in source_roots:
        for source in sorted((root / relative).rglob('*')):
            if source.is_file() and '__pycache__' not in source.parts:
                archive.write(source, source.relative_to(root).as_posix())
    for name in ['CMakeLists.txt', 'platformio.ini', 'sdkconfig.defaults', 'sdkconfig.esp32s3-skeleton', 'partitions.csv']:
        source = root / 'esp32s3' / name
        archive.write(source, source.relative_to(root).as_posix())

manifest = {
    'scope': 'Standalone MAME 0.220 MM IV/MM V core; no LCD/BLE/Wi-Fi frontend integration',
    'build': 22, 'flash': 12, 'original_speed_reached': False,
    'rom_image': json.loads((root / 'integration/chessengine/build-host/rom-image/manifest.json').read_text()),
    'sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(destination.iterdir()) if p.is_file()},
    'upstream_sources': {name: hashlib.sha256((root / 'ref/mame0220-src' / name).read_bytes()).hexdigest()
                         for name in ['src/emu/emumem.cpp', 'src/devices/cpu/m6502/m6502.cpp']},
}
(destination / 'checkpoint.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
print('Saved checkpoint:', destination)
print('Firmware SHA256:', manifest['sha256']['firmware.bin'])
