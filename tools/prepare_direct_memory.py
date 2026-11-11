"""Rewrite only CPU access call sites; leave upstream class definitions intact."""
import re
from pathlib import Path


def write_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding='utf-8') != text:
        path.write_text(text, encoding='utf-8', newline='\n')


def prepare_instructions(source, destination):
    original = source.read_text(encoding='utf-8')
    # Replace full token/call prefixes, retaining argument expressions verbatim.
    # In particular PC++ is evaluated exactly once at the original call site.
    replacements = {
        'read_pc()': 's3_direct_arg(mintf.get(), PC++)',
        'read_pc_noinc()': 's3_direct_arg(mintf.get(), PC)',
        'read(': 's3_direct_read(mintf.get(), ',
        'read_arg(': 's3_direct_arg(mintf.get(), ',
        'write(': 's3_direct_write(mintf.get(), ',
    }
    pattern = re.compile(r'\b(?:read_pc\(\)|read_pc_noinc\(\)|read\(|read_arg\(|write\()')
    counts = {}
    def replace(match):
        token = match.group()
        counts[token] = counts.get(token, 0) + 1
        return replacements[token]
    result = pattern.sub(replace, original)
    assert len(counts) == 5 and all(counts.values()), (source, counts)
    # Exact round trip proves all instruction/cycle/interrupt bodies are retained.
    restored = result
    for token, replacement in replacements.items():
        restored = restored.replace(replacement, token)
    assert restored == original, 'Instruction rewrite changed more than access calls'
    write_changed(destination, result)
    print(f'Direct memory instruction overlay: {source.name}: {counts}')


def prepare(base_text, destination):
    generated = destination.parent.resolve()
    component = generated.parent
    for name in ('m6502', 'm65c02'):
        prepare_instructions(component / f'include/cpu/m6502/{name}.hxx',
                             generated / f'{name}_direct_s3.hxx')
    marker = '#include "cpu/m6502/m6502.hxx"'
    assert base_text.count(marker) == 1
    base_text = base_text.replace(marker, '#include "m6502_direct_s3.hxx"')
    marker = '#include "../src/s3_fast_memory.h"'
    assert base_text.count(marker) == 1
    base_text = base_text.replace(marker, marker + '\n#include "../src/s3_direct_memory.h"')
    assert base_text.count('mintf->read_sync(PC)') == 2
    base_text = base_text.replace('mintf->read_sync(PC)', 's3_direct_opcode(mintf.get(), PC)')
    marker = 'std::unique_ptr<s3_fast_memory> pages;'
    assert base_text.count(marker) == 1
    base_text = base_text.replace(marker, marker + '''
        ~paged_memory_interface() override {
            if (s3_direct_memory.owner == this) s3_direct_memory = {};
        }''')
    marker = 'intf.pages = std::make_unique<s3_fast_memory>(*intf.program, *intf.sprogram);\n    }'
    assert base_text.count(marker) == 1
    base_text = base_text.replace(marker, marker.replace('\n    }', '''
        s3_direct_memory = {mintf.get(), intf.pages.get(), intf.data_cache.get(), intf.cache, intf.scache};
        std::printf("[MAME DIRECT MEMORY] qualified accesses enabled\\n");
    }'''))
    # The CMOS implementation includes its own generated instruction bodies.
    root = component.parents[2]
    cmos = root / 'ref/mame0220-src/src/devices/cpu/m6502/m65c02.cpp'
    text = cmos.read_text(encoding='utf-8')
    # Absolute includes retain the upstream relative-header lookup without
    # changing any include profiles for other translation units.
    for name in ('m65c02.h', 'm65c02d.h'):
        text = text.replace(f'#include "{name}"', f'#include "{(cmos.parent / name).as_posix()}"')
    marker = '#include "cpu/m6502/m65c02.hxx"'
    assert text.count(marker) == 1
    text = text.replace(marker, '#include "m65c02_direct_s3.hxx"')
    write_changed(generated / 'm65c02_direct_s3.cpp', text)
    return base_text
