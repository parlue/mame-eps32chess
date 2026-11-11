"""Rewrite qualified memory calls only in an explicit subset of IRAM routines."""
import json
import re


class HotDirectRewrite:
    def __init__(self, root, hot_routines):
        self.selected = set(json.loads((root / 'tools/65xx_hot_direct.json').read_text())['routines'])
        assert self.selected and self.selected <= hot_routines
        self.matched = set()

    def __call__(self, text):
        replacements = {
            'read_pc()': 's3_direct_arg(mintf.get(), PC++)',
            'read_pc_noinc()': 's3_direct_arg(mintf.get(), PC)',
            'read(': 's3_direct_read(mintf.get(), ',
            'read_arg(': 's3_direct_arg(mintf.get(), ',
            'write(': 's3_direct_write(mintf.get(), ',
        }
        pattern = re.compile(r'\b(?:read_pc\(\)|read_pc_noinc\(\)|read\(|read_arg\(|write\()')
        for name in sorted(self.selected):
            signature = f'void {name}()'
            if signature not in text:
                continue
            assert text.count(signature) == 1 and name not in self.matched, name
            start = text.index('{', text.index(signature))
            end, depth = start + 1, 1
            while depth:
                depth += (text[end] == '{') - (text[end] == '}')
                end += 1
            original = text[start + 1:end - 1]
            changed = pattern.sub(lambda match: replacements[match.group()], original)
            changed = changed.replace('mintf->read_sync(PC)', 's3_direct_opcode(mintf.get(), PC)')
            restored = changed.replace('s3_direct_opcode(mintf.get(), PC)', 'mintf->read_sync(PC)')
            for token, replacement in replacements.items():
                restored = restored.replace(replacement, token)
            assert changed != original and restored == original, f'Non-memory change in {name}'
            text = text[:start + 1] + changed + text[end - 1:]
            self.matched.add(name)
        return text

    def check(self):
        assert self.matched == self.selected, self.selected - self.matched
        print(f'Qualified inline memory selected for {len(self.matched)} IRAM routines; other bodies unchanged')


def attach_context(text):
    marker = '#include "../src/s3_fast_memory.h"'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '''
#define S3_DIRECT_MEMORY_FORCE_INLINE 1
#include "../src/s3_direct_memory.h"
#undef S3_DIRECT_MEMORY_FORCE_INLINE''')
    marker = 'std::unique_ptr<s3_fast_memory> pages;'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '''
        ~paged_memory_interface() override {
            if (s3_direct_memory.owner == static_cast<memory_interface *>(this)) s3_direct_memory = {};
        }''')
    marker = 'intf.pages = std::make_unique<s3_fast_memory>(*intf.program, *intf.sprogram);\n    }'
    assert text.count(marker) == 1
    text = text.replace(marker, marker.replace('\n    }', '''
        s3_direct_memory = {mintf.get(), intf.pages.get(), intf.data_cache.get(), intf.cache, intf.scache};
        std::printf("[MAME HOT DIRECT] qualified selected accesses enabled\\n");
    }'''))
    return text
