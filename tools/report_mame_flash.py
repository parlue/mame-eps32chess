"""Rank MAME archive contributions actually mapped into S3 flash.

Uses allocated input sections after the GNU ld map marker. Discarded input
sections and ELF debug information are excluded. Counts are section bytes,
not a prediction of savings or a complete firmware-image size measurement.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('map', type=Path)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
text = args.map.read_text(errors='replace').split('Linker script and memory map', 1)[1]
contributions = Counter()
seen = set()
for line in text.splitlines():
    match = re.match(r'^\s+(?:\.[\w.$]+\s+)?(0x[\da-f]+)\s+(0x[\da-f]+)\s+(.+)', line)
    if not match:
        continue
    address, size, owner = match.groups()
    address, size = int(address, 16), int(size, 16)
    if 'libmame0220.a(' not in owner:
        continue
    # ESP32-S3 memory-mapped flash data and instruction windows only.
    if not (0x3c000000 <= address < 0x3e000000 or 0x42000000 <= address < 0x44000000):
        continue
    member = owner.split('libmame0220.a(', 1)[1].split(')', 1)[0]
    key = (member, address, size)
    if key in seen:
        continue
    seen.add(key)
    contributions[member] += size
assert contributions, 'No mapped MAME sections found'
report = {'measurement': 'MAME input sections in mapped S3 flash windows; excludes RAM/debug/discarded sections',
          'total_section_bytes': sum(contributions.values()),
          'objects': [{'object': name, 'bytes': size} for name, size in contributions.most_common()]}
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
for name, size in contributions.most_common(20):
    print(f'{size:9,d}  {name}')
