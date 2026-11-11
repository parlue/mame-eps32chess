"""Compare explicit idle heap snapshots from the S3 diagnostic console."""
import argparse
import re
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('log', type=Path)
p.add_argument('--before', type=int, default=1)
p.add_argument('--after', type=int, default=3)
a = p.parse_args()
snapshots = []
current = None
for line in a.log.read_text(encoding='utf-8').splitlines():
    if line == '[HEAP BEGIN]':
        current = {}
    elif line == '[HEAP END]' and current is not None:
        snapshots.append(current)
        current = None
    elif current is not None:
        match = re.fullmatch(r'\[HEAP BLOCK\] (0x[0-9a-fA-F]+) (\d+) ([0-9a-f]+)', line)
        if match:
            current[int(match[1], 16)] = (int(match[2]), bytes.fromhex(match[3]))
print('Completed snapshots:', len(snapshots))
if max(a.before, a.after) >= len(snapshots):
    raise SystemExit('Requested snapshot not yet present')
before, after = snapshots[a.before], snapshots[a.after]
for label, source, other in [('ADDED', after, before), ('REMOVED', before, after)]:
    count = total = 0
    for address, value in sorted(source.items()):
        if address in other and value == other[address]:
            continue
        size, data = value
        count += 1
        total += size
        printable = ''.join(chr(c) if 32 <= c < 127 else '.' for c in data)
        print(label, hex(address), size, data.hex(), repr(printable))
    print(label, 'blocks=', count, 'payload bytes=', total)
