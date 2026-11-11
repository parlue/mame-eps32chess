"""Keep the observed 16/32-bit address roots of the native-8 LE chess profile.

This restricts supported address-space configurations, not logical access sizes.
The actual 16-bit CPU space and full 32-bit dummy space remain unchanged.
Other address widths require an explicitly extended or disabled root profile.
Input is the owning, native-8 allocator overlay; upstream stays untouched.
"""
import argparse
from pathlib import Path
import re


def prepare(source: Path, destination: Path):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    assert '65xx native8/LE' in text, 'Native-8 LE allocator profile required'
    assert '~address_space_specific() override' in text, 'Owning overlay required'
    pattern = r'\t\tswitch \(address_width\) \{\n(.*?)\n\t\t\}'
    matches = list(re.finditer(pattern, text, re.S))
    assert len(matches) == 1, 'Unexpected address-root switch'
    match = matches[0]
    lines = match[1].splitlines()
    cases = [(int(m[1]), line) for line in lines
             if (m := re.match(r'\s*case\s+(\d+):', line))]
    assert [number for number, _ in cases] == list(range(1, 33)), 'Unexpected roots'
    kept = [line for number, line in cases if number in (16, 32)]
    assert all('handler_entry_read_dispatch<' in line and
               'handler_entry_write_dispatch<' in line for line in kept)
    defaults = [line for line in lines if 'default: fatalerror(' in line]
    assert len(defaults) == 1
    replacement = '\t\tswitch (address_width) {\n' + '\n'.join(kept + defaults) + '\n\t\t}'
    text = text[:match.start()] + replacement + text[match.end():]
    # Reject before allocating handlers or calculating the address mask.
    anchor = '\t\tm_unmap_r = new handler_entry_read_unmapped'
    assert text.count(anchor) == 1
    guard = '''\t\tif (address_width != 16 && address_width != 32)
\t\t\tthrow emu_fatalerror("65xx root profile rejects address width %d (supports 16/32)", address_width);
'''
    text = text.replace(anchor, guard + anchor)
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('65xx address-root overlay prepared: CPU 16-bit / dummy 32-bit; other roots rejected')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
