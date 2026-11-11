"""Specialize the memory allocator overlay for the current 8-bit LE CPUs.

Preserves all address widths, mapping semantics and wide access operations.
Only unsupported native bus types stop pulling their dispatch specializations
into the image. A later CPU profile must explicitly add its required bus type.
Input is the generated ownership-fixed overlay, never the upstream checkout.
"""
import argparse
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('overlay', type=Path)
a = p.parse_args()
source = a.overlay.read_text(encoding='utf-8')
anchor = 'void memory_manager::allocate(device_memory_interface &memory)\n{'
assert source.count(anchor) == 1, 'Unexpected memory allocator source'
start = source.index(anchor)
end = source.index('\n}\n', start) + 3
original = source[start:end]
assert 'case 64|(4-3):' in original and 'case  8|(4-0):' in original
replacement = '''void memory_manager::allocate(device_memory_interface &memory)
{
    for (int spacenum = 0; spacenum < memory.max_space_count(); ++spacenum)
    {
        address_space_config const *const config = memory.space_config(spacenum);
        if (!config)
            continue;
        if (config->data_width() != 8 || config->addr_shift() != 0 ||
                config->endianness() != ENDIANNESS_LITTLE)
            throw emu_fatalerror("65xx profile rejects native bus width %d/shift %d/endian %d",
                config->data_width(), config->addr_shift(), int(config->endianness()));
        osd_printf_info("[MEM PROFILE] 65xx native8/LE, address=%d, space=%d\\n",
            config->addr_width(), spacenum);
        memory.allocate<address_space_specific<0, 0, ENDIANNESS_LITTLE>>(*this, spacenum);
    }
}
'''
source = source[:start] + replacement + source[end:]
a.overlay.write_text(source, encoding='utf-8', newline='\n')
print('65xx allocator overlay prepared (native 8-bit / shift 0 / little endian)')
