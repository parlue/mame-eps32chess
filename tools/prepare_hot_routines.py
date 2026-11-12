"""Place explicitly selected CPU routines in IRAM without changing their bodies."""
import json
import re
from prepare_direct_memory import write_changed


def prepare(base_text, destination, direct=False, memory=False):
    generated = destination.parent.resolve()
    component = generated.parent
    root = component.parents[2]
    plan = json.loads((root / 'tools/65xx_hot_routines.json').read_text())
    selected = set(plan['routines'])
    assert len(selected) == len(plan['routines']) and selected
    assert all(re.fullmatch(r'm(?:6502|65c02)_device::\w+', name) for name in selected)
    matched = set()
    marker = 'IRAM_ATTR __attribute__((noinline)) '
    rewrite = lambda text: text
    if direct:
        from prepare_hot_direct import HotDirectRewrite, attach_context
        rewrite = HotDirectRewrite(root, selected)
        base_text = attach_context(base_text)

    def annotate(text):
        original = text
        for routine in sorted(selected):
            signature = f'void {routine}()'
            count = text.count(signature)
            if count:
                assert count == 1 and routine not in matched, routine
                text = text.replace(signature, f'void {marker}{routine}()')
                matched.add(routine)
        assert text.replace(marker, '') == original, 'Only function placement may change'
        return text

    base_text = annotate(rewrite(base_text))
    if memory:
        start = base_text.index('    class paged_memory_interface final : public cached_memory_interface {')
        end = base_text.index('\n    };', start)
        original = base_text[start:end]
        changed = original
        for signature in ('uint8_t read(uint16_t adr) override',
                          'void write(uint16_t adr, uint8_t val) override',
                          'uint8_t read_arg(uint16_t adr) override',
                          'uint8_t read_sync(uint16_t adr) override'):
            assert changed.count(signature) == 1, signature
            changed = changed.replace(signature, marker + signature)
        assert changed.replace(marker, '') == original, 'Memory bodies must remain unchanged'
        base_text = base_text[:start] + changed + base_text[end:]
        print('IRAM placement overlay: four shared paged-memory methods; bodies unchanged')
    base_text = base_text.replace('#include "../src/s3_fast_memory.h"',
                                  '#include "../src/s3_fast_memory.h"\n#include "esp_attr.h"')
    for name in ('m6502', 'm65c02'):
        original = (component / f'include/cpu/m6502/{name}.hxx').read_text()
        write_changed(generated / f'{name}_hot_s3.hxx', annotate(rewrite(original)))
    cmos = root / 'ref/mame0220-src/src/devices/cpu/m6502/m65c02.cpp'
    text = annotate(rewrite(cmos.read_text()))
    for name in ('m65c02.h', 'm65c02d.h'):
        text = text.replace(f'#include "{name}"', f'#include "{(cmos.parent / name).as_posix()}"')
    old = '#include "cpu/m6502/m65c02.hxx"'
    assert text.count(old) == 1
    write_changed(generated / 'm65c02_hot_s3.cpp', text.replace(old, '#include "m65c02_hot_s3.hxx"'))
    old = '#include "cpu/m6502/m6502.hxx"'
    assert base_text.count(old) == 1
    assert matched == selected, f'Missing selected routine definitions: {selected - matched}'
    if direct:
        rewrite.check()
    print(f'IRAM placement overlay prepared: {len(selected)} CPU routines; bodies unchanged')
    return base_text.replace(old, '#include "m6502_hot_s3.hxx"')
