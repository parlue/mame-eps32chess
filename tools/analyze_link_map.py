"""Group retained flash input sections by object, without loading the huge map."""
from collections import defaultdict
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
path=root/'esp32s3/.pio/build/esp32s3-skeleton/mame_s3.map'
objects=defaultdict(int)
active=False;pending=None
section=re.compile(r'^\s+(\.(?:text|literal|rodata)\S*)\s*(.*)$')
address=re.compile(r'^\s*0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)\s+(.+)$')
with path.open(encoding='utf-8',errors='replace') as f:
    for line in f:
        if 'Linker script and memory map' in line:active=True
        if not active:continue
        found=section.match(line)
        if found:
            pending=found.group(1)
            data=address.match(found.group(2))
        else:data=address.match(line) if pending else None
        if data:
            name=data.group(2).strip()
            if '.o' in name:objects[name]+=int(data.group(1),16)
            pending=None
        elif line.strip() and not found:pending=None
lines=['Retained text/literal/rodata by object (link map, not firmware image size):']
for name,size in sorted(objects.items(),key=lambda pair:pair[1],reverse=True)[:30]:
    lines.append(f'{size:9d} {name}')
text='\n'.join(lines)+'\n'
(root/'docs/takeover-flash-objects.txt').write_text(text,encoding='utf-8')
print(text)
