#pragma once

// Only plain upstream fixed-memory or bank handlers qualify. Other handlers remain
// on MAME's normal path. Bank::base() is re-read on every access, so set_entry
// and set_base remain visible even without an address-space change notification.
#include "emumem_hea.h"
#include "emumem_hem.h"
#include <array>
#include <cstdint>

class s3_fast_memory {
    using cache_type = memory_access_cache<0, 0, ENDIANNESS_LITTLE>;
    using read_bank = handler_entry_read_memory_bank<0, 0, ENDIANNESS_LITTLE>;
    using write_bank = handler_entry_write_memory_bank<0, 0, ENDIANNESS_LITTLE>;
    struct page {
        memory_bank *bank = nullptr;
        u8 *fixed = nullptr;
        u32 offset = 0;
        u8 *base() const { return bank ? static_cast<u8 *>(bank->base()) : fixed; }
    };
    using table = std::array<page, 256>;
    address_space &m_program, &m_opcodes;
    table m_reads{}, m_writes{};
    std::unique_ptr<table> m_opcode_reads;
    int m_program_notifier = -1, m_opcode_notifier = -1;
    bool m_valid = false;

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
                if (contiguous) pages[address >> 8] = page{bank, fixed ? reinterpret_cast<u8 *>(first) : nullptr, u32(first - base)};
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
        if (m_valid) std::printf("[MAME FAST MEMORY] invalidated; normal MAME fallback\n");
        m_valid = false;
    }
    u8 read_from(const table &pages, u16 address, cache_type &fallback) const {
        if (m_valid) {
            const auto &p = pages[address >> 8];
            if (auto *base = p.base()) return base[p.offset + (address & 255)];
        }
        return fallback.read_byte(address);
    }
public:
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
        unsigned reads = 0, writes = 0;
        for (const auto &p : m_reads) reads += bool(p.bank || p.fixed);
        for (const auto &p : m_writes) writes += bool(p.bank || p.fixed);
        std::printf("[MAME FAST MEMORY] read_pages=%u write_pages=%u bytes=%u opcode_shared=%d\n",
            reads, writes, unsigned(sizeof(*this) + (m_opcode_reads ? sizeof(table) : 0)), int(&program == &opcodes));
    }
    ~s3_fast_memory() {
        if (m_program_notifier >= 0) m_program.remove_change_notifier(m_program_notifier);
        if (m_opcode_notifier >= 0) m_opcodes.remove_change_notifier(m_opcode_notifier);
    }
    s3_fast_memory(const s3_fast_memory &) = delete;
    s3_fast_memory &operator=(const s3_fast_memory &) = delete;
    // Startup-only probe: expose only already-qualified, side-effect-free bytes.
    const u8 *probe_read_pointer(u16 address) const {
        if (!m_valid) return nullptr;
        const auto &p = m_reads[address >> 8];
        auto *base = p.base();
        return base ? base + p.offset + (address & 255) : nullptr;
    }
    u8 read(u16 address, cache_type &fallback) const { return read_from(m_reads, address, fallback); }
    u8 opcode(u16 address, cache_type &fallback) const { return read_from(m_opcode_reads ? *m_opcode_reads : m_reads, address, fallback); }
    void write(u16 address, u8 value, cache_type &fallback) const {
        if (m_valid) {
            const auto &p = m_writes[address >> 8];
            if (auto *base = p.base()) { base[p.offset + (address & 255)] = value; return; }
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
            if (m_reads[i].bank || m_reads[i].fixed) for (unsigned j = 0; j < 256; ++j) {
                const u16 address = u16((i << 8) | j);
                if (read(address, data) != data.read_byte(address)) throw emu_fatalerror("Fast memory read self-test failed");
                ++checked;
            }
            const auto r = reinterpret_cast<uintptr_t>(m_reads[i].base());
            const auto w = reinterpret_cast<uintptr_t>(m_writes[i].base());
            if (r && w && r + m_reads[i].offset == w + m_writes[i].offset) scratch = (i << 8) | 255;
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
        std::printf("[MAME FAST TEST] bytes=%u read/write=OK invalidation/tap=OK restored=OK\n", checked);
    }
};
