"""Instrument 6502 execution and compare standard versus cached data access."""
import argparse
from pathlib import Path
from prepare_no_desktop_text import replace_body


def prepare(source, destination):
    text = source.read_text(encoding='utf-8')
    assert source.resolve() != destination.resolve()
    assert '~owned_memory_interface() override { delete cache; delete scache; }' in text
    anchor = 'DEFINE_DEVICE_TYPE(M6502,'
    assert text.count(anchor) == 1
    support = '''#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>

namespace {
std::atomic<unsigned> s3_speed_mode{0};
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
    anchor = '\tmintf = std::make_unique<owned_memory_interface>();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, '''    class cached_memory_interface final : public mi_default {
    public:
        cached_memory_interface() { cache = nullptr; scache = nullptr; }
        ~cached_memory_interface() override { delete cache; delete scache; }
        uint8_t read(uint16_t adr) override { return cache->read_byte(adr); }
        void write(uint16_t adr, uint8_t val) override { cache->write_byte(adr, val); }
    };
    if (s3_speed_mode.load() & 1) mintf = std::make_unique<cached_memory_interface>();
    else mintf = std::make_unique<owned_memory_interface>();''')
    anchor = '\tmintf->scache = mintf->sprogram->cache<0, 0, ENDIANNESS_LITTLE>();'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + '''
    s3_cpu_us = 0; s3_cpu_calls = s3_cpu_cycles = 0;
    std::printf("[MAME CPU MEMORY] cpu=%p bytes=%u interface=%p cache=%p scache=%p cache_bytes=%u cached=%d\\n",
        this, unsigned(sizeof(*this)), mintf.get(), mintf->cache, mintf->scache,
        unsigned(sizeof(*mintf->cache)), int(s3_speed_mode.load() & 1));''')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('6502 speed comparison overlay prepared')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
