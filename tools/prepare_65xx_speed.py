"""Instrument 6502 execution and compare standard versus cached data access."""
import argparse
from pathlib import Path
from prepare_no_desktop_text import replace_body


def prepare(source, destination, memory_probe=False, direct_memory=False, opcode_profile=False, hot_routines=False, hot_direct=False, hot_memory=False):
    text = source.read_text(encoding='utf-8')
    assert source.resolve() != destination.resolve()
    assert '~owned_memory_interface() override { delete cache; delete scache; }' in text
    # resolve_safe() inserts a dummy function even when SYNC is unconnected.
    # Keep real connections, but avoid two pointless callbacks per instruction.
    assert text.count('sync_w.resolve_safe();') == 1
    assert text.count('sync_w(ASSERT_LINE);') == 2
    assert text.count('sync_w(CLEAR_LINE);') == 3
    text = text.replace('sync_w(ASSERT_LINE);', 'if (sync_w) sync_w(ASSERT_LINE);')
    text = text.replace('sync_w(CLEAR_LINE);', 'if (sync_w) sync_w(CLEAR_LINE);')
    text = text.replace('sync_w.resolve_safe();', '''sync_w.resolve();
    // Exercise the actual devcb API before CPU execution, including both edges.
    {
        auto empty = std::make_unique<devcb_write_line>(*this);
        empty->resolve();
        if (*empty) fatalerror("Unexpected connected empty SYNC probe");
        unsigned edges = 0;
        int first = -1, last = -1;
        auto connected = std::make_unique<devcb_write_line>(*this);
        connected->bind().set([&](int state) {
            if (!edges) first = state;
            ++edges; last = state;
        });
        connected->resolve();
        if (*connected) (*connected)(ASSERT_LINE);
        if (*connected) (*connected)(CLEAR_LINE);
        if (edges != 2 || first != ASSERT_LINE || last != CLEAR_LINE)
            fatalerror("Connected SYNC probe lost signal edges");
        std::printf("[MAME SYNC TEST] empty=OK connected_edges=2 first=1 last=0 actual_connected=%d\\n", int(bool(sync_w)));
    }''')
    anchor = 'DEFINE_DEVICE_TYPE(M6502,'
    assert text.count(anchor) == 1
    support = '''#include "../src/s3_fast_memory.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>

namespace {
std::atomic<unsigned> s3_speed_mode{5};
int64_t s3_cpu_us = 0;
uint64_t s3_cpu_calls = 0, s3_cpu_cycles = 0;
struct s3_execution_scope {
    int &remaining;
    int initial;
    int64_t start;
    explicit s3_execution_scope(int &count) : remaining(count), initial(count), start(esp_timer_get_time()) {}
    ~s3_execution_scope() {
        s3_cpu_us += esp_timer_get_time() - start;
        ++s3_cpu_calls;
        if (initial > remaining) s3_cpu_cycles += initial - remaining;
    }
};
}
extern "C" void s3_m6502_set_cache(bool enabled) { s3_speed_mode.store(enabled ? 1 : 0); }
extern "C" void s3_m6502_set_speed(unsigned mode) { s3_speed_mode.store(mode); }
extern "C" unsigned s3_m6502_speed_mode() { return s3_speed_mode.load(); }
extern "C" void s3_m6502_profile_report(int64_t elapsed) {
    std::printf("[MAME CPU] cached=%d wall=%.1f%% calls=%llu cycles=%llu stack_free=%u\\n",
        int(s3_speed_mode.load() & 1), 100.0 * s3_cpu_us / elapsed,
        (unsigned long long)s3_cpu_calls, (unsigned long long)s3_cpu_cycles,
        unsigned(uxTaskGetStackHighWaterMark(nullptr)));
    s3_cpu_us = 0; s3_cpu_calls = s3_cpu_cycles = 0;
}

'''
    text = text.replace(anchor, support + anchor)
    signature = 'void m6502_device::execute_run()'
    marker = signature + '\n{'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '\n\ts3_execution_scope profile_scope(icount);')
    text = replace_body(text, 'void m6502_device::device_start()', '''    class owned_memory_interface final : public mi_default {
    public:
        owned_memory_interface() { cache = nullptr; scache = nullptr; }
        ~owned_memory_interface() override { delete cache; delete scache; }
    };
    class cached_memory_interface : public mi_default {
    public:
        std::unique_ptr<memory_access_cache<0, 0, ENDIANNESS_LITTLE>> data_cache;
        cached_memory_interface() { cache = nullptr; scache = nullptr; }
        ~cached_memory_interface() override { delete cache; delete scache; }
        uint8_t read(uint16_t adr) override { return data_cache->read_byte(adr); }
        void write(uint16_t adr, uint8_t val) override { data_cache->write_byte(adr, val); }
    };
    class paged_memory_interface final : public cached_memory_interface {
    public:
        std::unique_ptr<s3_fast_memory> pages;
        uint8_t read(uint16_t adr) override { return pages->read(adr, *data_cache); }
        void write(uint16_t adr, uint8_t val) override { pages->write(adr, val, *data_cache); }
        uint8_t read_arg(uint16_t adr) override { return pages->read(adr, *cache); }
        uint8_t read_sync(uint16_t adr) override { return pages->opcode(adr, *scache); }
    };
    if (s3_speed_mode.load() & 4) mintf = std::make_unique<paged_memory_interface>();
    else if (s3_speed_mode.load() & 1) mintf = std::make_unique<cached_memory_interface>();
    else mintf = std::make_unique<owned_memory_interface>();
    init();
    // Upstream init registers state/caches but does not execute the CPU.
    // Initialize this independent lookup cache before reset/execution starts.
    if (s3_speed_mode.load() & 5) {
        auto &intf = static_cast<cached_memory_interface &>(*mintf);
        intf.data_cache.reset(intf.program->cache<0, 0, ENDIANNESS_LITTLE>());
        std::printf("[MAME DATA CACHE] data=%p operand=%p opcode=%p bytes=%u\\n",
            intf.data_cache.get(), intf.cache, intf.scache, unsigned(sizeof(*intf.data_cache)));
    }
    if (s3_speed_mode.load() & 4) {
        auto &intf = static_cast<paged_memory_interface &>(*mintf);
        intf.pages = std::make_unique<s3_fast_memory>(*intf.program, *intf.sprogram);
        intf.pages->self_test(*intf.data_cache);
        intf.pages.reset();
        intf.pages = std::make_unique<s3_fast_memory>(*intf.program, *intf.sprogram);
    }''')
    anchor = '\tmintf->scache = mintf->sprogram->cache<0, 0, ENDIANNESS_LITTLE>();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    s3_cpu_us = 0; s3_cpu_calls = s3_cpu_cycles = 0;
    std::printf("[MAME CPU MEMORY] cpu=%p bytes=%u interface=%p cache=%p scache=%p cache_bytes=%u cached=%d\\n",
        this, unsigned(sizeof(*this)), mintf.get(), mintf->cache, mintf->scache,
        unsigned(sizeof(*mintf->cache)), int(s3_speed_mode.load() & 1));''')
    if opcode_profile:
        marker = '#include <atomic>'
        text = text.replace(marker, marker + '\n#include "../src/s3_opcode_profile.h"')
        marker = '    s3_cpu_us = 0; s3_cpu_calls = s3_cpu_cycles = 0;'
        assert text.count(marker) == 2
        # One in the periodic reporter and one in CPU initialization.
        text = text.replace(marker, '    s3_opcode_report();\n' + marker, 1)
        index = text.index(marker, text.index('void m6502_device::init()'))
        text = text[:index] + '    s3_opcode_reset();\n' + text[index:]
        start = text.index('void m6502_device::execute_run()')
        end = text.index('void m6502_device::execute_set_input', start)
        body = text[start:end]
        assert body.count('if(inst_substate)\n\t\tdo_exec_partial();') == 1
        assert body.count('\t\tdo_exec_full();') == 1
        body = body.replace('if(inst_substate)\n\t\tdo_exec_partial();',
            'if(inst_substate) {\n\t\t++s3_opcode_partial;\n\t\tdo_exec_partial();\n\t}')
        body = body.replace('\t\tdo_exec_full();', '\t\ts3_opcode_begin(inst_state);\n\t\tdo_exec_full();')
        text = text[:start] + body + text[end:]
    if direct_memory:
        from prepare_direct_memory import prepare as prepare_direct
        text = prepare_direct(text, destination)
    if hot_routines or hot_direct or hot_memory:
        from prepare_hot_routines import prepare as prepare_hot
        text = prepare_hot(text, destination, hot_direct, hot_memory)
    if memory_probe:
        anchor = '#include <atomic>'
        text = text.replace(anchor, anchor + '\n#include "../src/s3_memory_probe.h"')
        anchor = 'intf.pages = std::make_unique<s3_fast_memory>(*intf.program, *intf.sprogram);\n    }'
        assert text.count(anchor) == 1
        text = text.replace(anchor, anchor.replace('\n    }', '\n        s3_memory_probe<memory_interface>::run(*mintf, *intf.data_cache, *intf.pages);\n    }'))
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('6502 speed comparison overlay prepared')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--memory-probe', action='store_true', help='Bounded startup-only RAM/ROM access comparison')
    parser.add_argument('--direct-memory', action='store_true', help='Qualified CPU accesses without virtual interface dispatch')
    parser.add_argument('--opcode-profile', action='store_true', help='Diagnostic instruction-start histogram, not speed measurements')
    parser.add_argument('--hot-routines', action='store_true', help='Explicitly selected CPU routine placement in internal instruction RAM')
    parser.add_argument('--hot-direct', action='store_true', help='Inline qualified memory only in a selected IRAM subset')
    parser.add_argument('--hot-memory', action='store_true', help='Place four shared paged-memory methods in internal instruction RAM')
    args = parser.parse_args()
    assert sum((args.memory_probe, args.direct_memory, args.opcode_profile, args.hot_routines, args.hot_direct, args.hot_memory)) <= 1, 'Run these comparison profiles separately'
    prepare(args.source, args.destination, args.memory_probe, args.direct_memory, args.opcode_profile, args.hot_routines, args.hot_direct, args.hot_memory)
