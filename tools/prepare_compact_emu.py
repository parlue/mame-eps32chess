"""Create a consistent core-source view with a smaller 32-bit dispatch fan-out.

All core headers and translation units use the same view. Upstream stays
untouched; unchanged files keep their timestamps across CMake runs.
"""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('upstream', type=Path)
p.add_argument('destination', type=Path)
a = p.parse_args()
source = a.upstream.resolve()
destination = a.destination.resolve()
assert source != destination and source not in destination.parents
assert (source / 'emumem.h').is_file()
old = b'\t\t(highbits > 32) ? 32 :\n\t\t(highbits > 14) ? 14 :'
new = b'\t\t(highbits > 32) ? 32 :\n\t\t(highbits > 24) ? 24 :\n\t\t(highbits > 14) ? 14 :'
count = 0
for original in sorted(source.rglob('*')):
    if not original.is_file():
        continue
    relative = original.relative_to(source)
    content = original.read_bytes()
    if relative.as_posix() == 'emumem.h':
        content = content.replace(b'\r\n', b'\n')
        assert content.count(old) == 1, 'Unexpected dispatch segmentation'
        content = content.replace(old, new)
    target = destination / relative
    if not target.exists() or target.read_bytes() != content:
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
    count += 1
print(f'Compact core view prepared: {count} files; full address widths retained')
