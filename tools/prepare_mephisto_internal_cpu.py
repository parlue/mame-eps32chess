"""Allocate the current Mephisto 65C02 object in reserved internal RAM."""
import argparse
from pathlib import Path


def prepare(source, destination):
    assert source.resolve() != destination.resolve()
    text = source.read_text(encoding='utf-8')
    anchor = 'void mephisto_state::rebel5(machine_config &config)'
    assert text.count(anchor) == 1
    support = '''#include "esp_heap_caps.h"
extern "C" unsigned s3_m6502_speed_mode();
namespace {
void *s3_cpu_reservation = nullptr;
struct s3_internal_cpu_creator {
    const std::type_info &type() const { return typeid(m65c02_device); }
    std::unique_ptr<m65c02_device> create(machine_config &config, const char *tag, device_t *owner, u32 clock) const {
        void *storage = nullptr;
        if (s3_m6502_speed_mode() & 2) {
            storage = s3_cpu_reservation;
            s3_cpu_reservation = nullptr;
            if (!storage) storage = heap_caps_malloc(sizeof(m65c02_device), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        }
        if (!storage) storage = ::operator new(sizeof(m65c02_device));
        // Match make_unique_clear's zero-initialization and original constructor.
        std::memset(storage, 0, sizeof(m65c02_device));
        std::printf("[MAME CPU ALLOC] mode=%u address=%p bytes=%u internal_free=%u\\n",
            s3_m6502_speed_mode(), storage, unsigned(sizeof(m65c02_device)),
            unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
        try { return std::unique_ptr<m65c02_device>(new(storage) m65c02_device(config, tag, owner, clock)); }
        catch (...) { ::operator delete(storage); throw; }
    }
};
}
extern "C" void s3_m65c02_reserve() {
    if (!s3_cpu_reservation) s3_cpu_reservation = heap_caps_malloc(sizeof(m65c02_device), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    std::printf("[MAME CPU RESERVE] address=%p bytes=%u\\n", s3_cpu_reservation, unsigned(sizeof(m65c02_device)));
}

'''
    text = text.replace(anchor, support + anchor)
    original = '\tM65C02(config, m_maincpu, 9.8304_MHz_XTAL / 2);'
    assert text.count(original) == 1
    text = text.replace(original, '''    auto target = m_maincpu.finder_target();
    assert(&config.current_device() == &target.first);
    m_maincpu = *config.device_add<s3_internal_cpu_creator, u32>(target.second,
        s3_internal_cpu_creator{}, u32((9.8304_MHz_XTAL / 2).value()));''')
    destination.parent.mkdir(parents=True, exist_ok=True)
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')
    print('Mephisto internal CPU allocation overlay prepared')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    prepare(args.source, args.destination)
