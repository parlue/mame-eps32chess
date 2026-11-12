#pragma once

// Only plain upstream fixed-memory or bank handlers qualify. Other handlers remain
// on MAME's normal path. Normally bank bases are re-read on every access; the
// flat-page profile instead invalidates before bank mutation and after restore.
#include "emumem_hea.h"
#include "emumem_hem.h"
#include <array>
#include <cstdint>
#ifdef BAUERINO_MAME_HOT_METADATA
#include "esp_attr.h"
#include "esp_memory_utils.h"
#endif

class s3_fast_memory {
    using cache_type = memory_access_cache<0, 0, ENDIANNESS_LITTLE>;
    using read_bank = handler_entry_read_memory_bank<0, 0, ENDIANNESS_LITTLE>;
    using write_bank = handler_entry_write_memory_bank<0, 0, ENDIANNESS_LITTLE>;
    struct page {
#ifdef BAUERINO_MAME_FLAT_PAGES
        u8 *fixed = nullptr;
        u8 *base() const { return fixed; }
        u8 *pointer(u16 address) const { return fixed ? fixed + (address & 255) : nullptr; }
#else
        memory_bank *bank = nullptr;
        u8 *fixed = nullptr;
        u32 offset = 0;
        u8 *base() const { return bank ? static_cast<u8 *>(bank->base()) : fixed; }
        u8 *pointer(u16 address) const { auto *b = base(); return b ? b + offset + (address & 255) : nullptr; }
#endif
    };
    using table = std::array<page, 256>;
    address_space &m_program, &m_opcodes;
    table m_reads{}, m_writes{};
    std::unique_ptr<table> m_opcode_reads;
    int m_program_notifier = -1, m_opcode_notifier = -1;
    bool m_valid = false;
#ifdef BAUERINO_MAME_FLAT_PAGES
    inline static s3_fast_memory *s_bank_clients = nullptr;
    s3_fast_memory *m_next_bank_client = nullptr;
    static_assert(sizeof(page) == sizeof(u8 *), "One direct pointer per page");
#endif

#ifdef BAUERINO_MAME_HOT_METADATA
    // One bounded slot, owned by at most one helper on the MAME worker. Entries
    // retain bank pointers (never snapshot bank bases or actual guest RAM).
    struct hot_state {
        const s3_fast_memory *owner;
        bool valid;
        std::array<page, 8> reads, writes;
    };
    static hot_state &hot() {
        static DRAM_ATTR hot_state state{};
        static_assert(sizeof(state) <= 208, "Bounded internal metadata slot");
        return state;
    }
    void claim_hot() {
        auto &h = hot();
        if (h.owner) return; // Additional CPUs/helpers retain the original path.
        if (!esp_ptr_internal(&h)) throw emu_fatalerror("Hot metadata outside internal SRAM");
        std::copy_n(m_reads.begin(), h.reads.size(), h.reads.begin());
        std::copy_n(m_writes.begin(), h.writes.size(), h.writes.begin());
        h.valid = m_valid;
        h.owner = this;
        std::printf("[MAME HOT METADATA] pages=8 bytes=%u internal=1\n", unsigned(sizeof(h)));
    }
#endif

    static memory_bank *find_bank(address_space &space, const handler_entry &entry) {
        const auto name = entry.name();
        for (const auto &item : space.device().machine().memory().banks()) {
            auto *bank = item.second.get();
            if (bank->references_space(space, read_or_write::READWRITE) && name == bank->name()) return bank;
        }
        return nullptr;
    }
    template<class Handler>
    static void populate(address_space &space, const std::vector<memory_entry> &map, table &pages) {
        using Fixed = std::conditional_t<std::is_same<Handler, read_bank>::value,
            handler_entry_read_memory<0, 0, ENDIANNESS_LITTLE>, handler_entry_write_memory<0, 0, ENDIANNESS_LITTLE>>;
        using Access = std::conditional_t<std::is_same<Handler, read_bank>::value,
            handler_entry_read<0, 0, ENDIANNESS_LITTLE>, handler_entry_write<0, 0, ENDIANNESS_LITTLE>>;
        for (const auto &entry : map) {
            const bool fixed = typeid(*entry.entry) == typeid(Fixed);
            if (entry.start > 0xffff || (!fixed && typeid(*entry.entry) != typeid(Handler))) continue;
            auto *bank = fixed ? nullptr : find_bank(space, *entry.entry);
            if (!fixed && (!bank || !bank->base())) continue;
            auto *handler = static_cast<Access *>(entry.entry);
            const u32 limit = std::min<u32>(entry.end, 0xffff);
            for (u32 address = (entry.start + 255) & ~255U; address + 255 <= limit; address += 256) {
                const auto first = reinterpret_cast<uintptr_t>(handler->get_ptr(address));
                const auto base = fixed ? first : reinterpret_cast<uintptr_t>(bank->base());
                if (!first || first < base) continue;
                bool contiguous = true;
                for (u32 i = 1; i < 256; ++i)
                    if (reinterpret_cast<uintptr_t>(handler->get_ptr(address + i)) != first + i) { contiguous = false; break; }
                if (contiguous) {
#ifdef BAUERINO_MAME_FLAT_PAGES
                    pages[address >> 8] = page{reinterpret_cast<u8 *>(first)};
#else
                    pages[address >> 8] = page{bank, fixed ? reinterpret_cast<u8 *>(first) : nullptr, u32(first - base)};
#endif
                }
            }
        }
    }
    static void build(address_space &space, table &reads, table *writes) {
        // The CPU adapter is specifically the 16-bit, native byte LE bus.
        if (space.addrmask() != 0xffff || space.data_width() != 8) return;
        std::vector<memory_entry> read_map, write_map;
        space.dump_maps(read_map, write_map);
        populate<read_bank>(space, read_map, reads);
        if (writes) populate<write_bank>(space, write_map, *writes);
    }
    void invalidate() {
#ifdef BAUERINO_MAME_HOT_METADATA
        if (hot().owner == this) hot().valid = false;
#endif
        if (m_valid) std::printf("[MAME FAST MEMORY] invalidated; normal MAME fallback\n");
        m_valid = false;
    }
    u8 read_from(const table &pages, u16 address, cache_type &fallback) const {
#ifdef BAUERINO_MAME_HOT_METADATA
        if (address < 0x800 && &pages == &m_reads && hot().owner == this && hot().valid) {
            const auto &p = hot().reads[address >> 8];
            if (auto *value = p.pointer(address)) return *value;
        }
#endif
        if (m_valid) {
            const auto &p = pages[address >> 8];
            if (auto *value = p.pointer(address)) return *value;
        }
        return fallback.read_byte(address);
    }
public:
#ifdef BAUERINO_MAME_FLAT_PAGES
    static void banks_changed() {
        for (auto *p = s_bank_clients; p; p = p->m_next_bank_client) p->invalidate();
    }
#endif
    s3_fast_memory(address_space &program, address_space &opcodes) : m_program(program), m_opcodes(opcodes) {
        build(program, m_reads, &m_writes);
        if (&program != &opcodes) {
            m_opcode_reads = std::make_unique<table>();
            build(opcodes, *m_opcode_reads, nullptr);
        }
        try {
            m_program_notifier = program.add_change_notifier([this](read_or_write) { invalidate(); });
            if (&program != &opcodes) m_opcode_notifier = opcodes.add_change_notifier([this](read_or_write) { invalidate(); });
        } catch (...) {
            if (m_program_notifier >= 0) program.remove_change_notifier(m_program_notifier);
            throw;
        }
        m_valid = true;
#ifdef BAUERINO_MAME_FLAT_PAGES
        m_next_bank_client = s_bank_clients;
        s_bank_clients = this;
        std::printf("[MAME FLAT PAGES] pointer_bytes=%u bank_changes=fallback\n", unsigned(sizeof(page)));
#endif
#ifdef BAUERINO_MAME_HOT_METADATA
        claim_hot();
#endif
        unsigned reads = 0, writes = 0;
        for (const auto &p : m_reads) reads += bool(p.base());
        for (const auto &p : m_writes) writes += bool(p.base());
        std::printf("[MAME FAST MEMORY] read_pages=%u write_pages=%u bytes=%u opcode_shared=%d\n",
            reads, writes, unsigned(sizeof(*this) + (m_opcode_reads ? sizeof(table) : 0)), int(&program == &opcodes));
    }
    ~s3_fast_memory() {
#ifdef BAUERINO_MAME_FLAT_PAGES
        auto **link = &s_bank_clients;
        while (*link && *link != this) link = &(*link)->m_next_bank_client;
        if (*link) *link = m_next_bank_client;
#endif
#ifdef BAUERINO_MAME_HOT_METADATA
        if (hot().owner == this) { hot().valid = false; hot().owner = nullptr; }
#endif
        if (m_program_notifier >= 0) m_program.remove_change_notifier(m_program_notifier);
        if (m_opcode_notifier >= 0) m_opcodes.remove_change_notifier(m_opcode_notifier);
    }
    s3_fast_memory(const s3_fast_memory &) = delete;
    s3_fast_memory &operator=(const s3_fast_memory &) = delete;
    // Startup-only probe: expose only already-qualified, side-effect-free bytes.
    const u8 *probe_read_pointer(u16 address) const {
        if (!m_valid) return nullptr;
        const auto &p = m_reads[address >> 8];
        return p.pointer(address);
    }
    u8 read(u16 address, cache_type &fallback) const { return read_from(m_reads, address, fallback); }
    u8 opcode(u16 address, cache_type &fallback) const { return read_from(m_opcode_reads ? *m_opcode_reads : m_reads, address, fallback); }
    void write(u16 address, u8 value, cache_type &fallback) const {
#ifdef BAUERINO_MAME_HOT_METADATA
        if (address < 0x800 && hot().owner == this && hot().valid) {
            const auto &p = hot().writes[address >> 8];
            if (auto *target = p.pointer(address)) { *target = value; return; }
        }
#endif
        if (m_valid) {
            const auto &p = m_writes[address >> 8];
            if (auto *target = p.pointer(address)) { *target = value; return; }
        }
        fallback.write_byte(address, value);
    }
    // Real upstream-handler comparisons on side-effect-free bank pages only.
    // A temporary RAM tap verifies notification + fallback, then is removed.
    // The caller constructs a fresh table afterward, outside any remap callback.
    void self_test(cache_type &data) {
        unsigned checked = 0;
        int scratch = -1;
        for (unsigned i = 0; i < 256; ++i) {
            if (m_reads[i].base()) for (unsigned j = 0; j < 256; ++j) {
                const u16 address = u16((i << 8) | j);
                if (read(address, data) != data.read_byte(address)) throw emu_fatalerror("Fast memory read self-test failed");
                ++checked;
            }
            const auto r = m_reads[i].pointer(u16(i << 8));
            const auto w = m_writes[i].pointer(u16(i << 8));
            if (r && r == w) {
#ifdef BAUERINO_MAME_HOT_METADATA
                // Exercise the internal slot's read/write and tap invalidation.
                if (scratch < 0 && i < 8) scratch = (i << 8) | 255;
#else
                scratch = (i << 8) | 255;
#endif
            }
        }
        if (scratch < 0 || !checked) throw emu_fatalerror("Fast memory self-test found no RAM/ROM pages");
        const u16 address = u16(scratch);
        const u8 saved = data.read_byte(address);
        write(address, 0x55, data);
        const bool fast_write_ok = data.read_byte(address) == 0x55;
        data.write_byte(address, 0xaa);
        const bool fast_read_ok = read(address, data) == 0xaa;
        data.write_byte(address, saved);
        if (!fast_write_ok || !fast_read_ok) throw emu_fatalerror("Fast memory write self-test failed");
        unsigned hits = 0;
        auto *tap = m_program.install_read_tap(address, address, "s3_fast_probe",
            std::function<void(offs_t, u8 &, u8)>([&hits](offs_t, u8 &, u8) { ++hits; }));
        const bool invalidated = !m_valid;
        const u8 observed = read(address, data);
        tap->remove();
        if (!invalidated || hits != 1 || observed != saved) throw emu_fatalerror("Fast memory tap fallback self-test failed");
#ifdef BAUERINO_MAME_FLAT_PAGES
        // A distinct fresh helper proves bank notification independently of
        // the map/tap test above; no guest data or bank selection is changed.
        auto observer = std::make_unique<s3_fast_memory>(m_program, m_opcodes);
        bool bank_test = false;
        for (const auto &item : m_program.device().machine().memory().banks()) {
            auto &bank = *item.second;
            if (bank.references_space(m_program, read_or_write::READWRITE) && bank.base()) {
                bank.set_base(bank.base());
                if (observer->m_valid || observer->read(address, data) != saved)
                    throw emu_fatalerror("Flat pages bank notification self-test failed");
                bank_test = true;
                break;
            }
        }
        if (!bank_test) throw emu_fatalerror("No bank for flat pages notification self-test");
        std::printf("[MAME FLAT BANK TEST] set_base=OK invalidation=OK fallback=OK\n");
#endif
        std::printf("[MAME FAST TEST] bytes=%u read/write=OK invalidation/tap=OK restored=OK\n", checked);
    }
};
#ifdef BAUERINO_MAME_FLAT_PAGES
extern "C" void s3_flat_bank_changed() { s3_fast_memory::banks_changed(); }
#endif
