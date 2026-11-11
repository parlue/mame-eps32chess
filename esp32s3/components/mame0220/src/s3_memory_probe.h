#pragma once
// Bounded startup microbenchmark. No instrumentation in live CPU accesses.
// All thunks retain the same extra harness call; reported costs include it.
template<class Interface> struct s3_memory_probe {
    using cache_type = memory_access_cache<0, 0, ENDIANNESS_LITTLE>;
    struct sample { u16 address; const u8 *raw; };
    struct context { Interface &intf; cache_type &cache; s3_fast_memory &pages; };
    using reader = u8 (*)(context &, const sample &);
    static __attribute__((noipa)) u8 raw(context &, const sample &s) { return *s.raw; }
    static __attribute__((noipa)) u8 cached(context &c, const sample &s) { return c.cache.read_byte(s.address); }
    static __attribute__((noipa)) u8 paged(context &c, const sample &s) { return c.pages.read(s.address, c.cache); }
    static __attribute__((noipa)) u8 data(context &c, const sample &s) { return c.intf.read(s.address); }
    static __attribute__((noipa)) u8 operand(context &c, const sample &s) { return c.intf.read_arg(s.address); }
    static __attribute__((noipa)) u8 opcode(context &c, const sample &s) { return c.intf.read_sync(s.address); }
    static void run(Interface &intf, cache_type &cache, s3_fast_memory &pages) {
        if (intf.program != intf.sprogram)
            throw emu_fatalerror("Memory probe requires shared opcode/data space");
        auto page_ids = std::make_unique<std::array<u16, 256>>();
        auto samples = std::make_unique<std::array<sample, 256>>();
        unsigned count = 0;
        for (unsigned page = 0; page < 256; ++page)
            if (pages.probe_read_pointer(u16(page << 8)) && pages.probe_read_pointer(u16((page << 8) | 255)))
                (*page_ids)[count++] = u16(page);
        if (!count) throw emu_fatalerror("No qualified memory probe pages");
        context ctx{intf, cache, pages};
        const reader paths[] = {raw, cached, paged, data, operand, opcode};
        const char *names[] = {"raw", "cache", "pages", "virtual_data", "virtual_operand", "virtual_opcode"};
        std::printf("[MAME MEMORY PROBE] begin pages=%u samples=256 blocks=64 rounds=3 shared_opcode=1\n", count);
        for (unsigned pattern = 0; pattern < 2; ++pattern) {
            u32 expected = 0;
            for (unsigned i = 0; i < 256; ++i) {
                const u16 address = u16(((*page_ids)[i * count / 256] << 8) | ((i * 73) & 255));
                const u8 *raw_ptr = pages.probe_read_pointer(address);
                if (!raw_ptr || *raw_ptr != cache.read_byte(address)) throw emu_fatalerror("Probe sample mismatch");
                (*samples)[i] = sample{address, raw_ptr};
                expected += *raw_ptr;
            }
            if (pattern) {
                u32 state = 0x65026502;
                for (unsigned i = 255; i; --i) {
                    state = state * 1664525U + 1013904223U;
                    std::swap((*samples)[i], (*samples)[state % (i + 1)]);
                }
            }
            expected *= 64;
            for (unsigned round = 0; round < 3; ++round) for (unsigned slot = 0; slot < 6; ++slot) {
                const unsigned method = (slot + round * 2) % 6;
                const reader read = paths[method];
                // Warm the same thunk/data before collecting timings.
                u32 warm = 0;
                for (const auto &s : *samples) warm += read(ctx, s);
                if (warm * 64 != expected) throw emu_fatalerror("Probe warm checksum mismatch");
                int64_t elapsed = 0;
                u32 checksum = 0;
                for (unsigned block = 0; block < 64; ++block) {
                    vTaskDelay(1); // Outside the timed block, keep watchdog/idle alive.
                    const int64_t start = esp_timer_get_time();
                    for (const auto &s : *samples) checksum += read(ctx, s);
                    elapsed += esp_timer_get_time() - start;
                }
                if (checksum != expected) throw emu_fatalerror("Memory probe checksum mismatch");
                std::printf("[MAME MEMORY PROBE] pattern=%s method=%s round=%u reads=16384 us=%lld checksum=%u\n",
                    pattern ? "shuffled" : "ordered", names[method], round, (long long)elapsed, unsigned(checksum));
            }
        }
        std::printf("[MAME MEMORY PROBE] PASS read_only=1 checksums=OK\n");
    }
};
