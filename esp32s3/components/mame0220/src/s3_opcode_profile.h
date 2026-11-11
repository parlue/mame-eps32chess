#pragma once

// Diagnostic only. Count instruction starts at the full-dispatch boundary;
// partial calls resume an already counted instruction and are counted separately.
// The selected machines execute on one MAME worker. These are frequencies, not
// per-routine wall times, and this instrumented binary is not a speed baseline.
namespace {
uint32_t s3_opcode_counts[256]{};
uint64_t s3_opcode_full = 0, s3_opcode_partial = 0, s3_opcode_special = 0;
unsigned s3_opcode_sequence = 0;

inline void s3_opcode_begin(unsigned state) {
    if (state < 256) { ++s3_opcode_counts[state]; ++s3_opcode_full; }
    else ++s3_opcode_special;
}
void s3_opcode_reset() {
    std::fill(std::begin(s3_opcode_counts), std::end(s3_opcode_counts), 0);
    s3_opcode_full = s3_opcode_partial = s3_opcode_special = 0;
    s3_opcode_sequence = 0;
}
void s3_opcode_report() {
    const auto sequence = ++s3_opcode_sequence;
    std::printf("[MAME OPCODE TOTAL] seq=%u full=%llu partial=%llu special=%llu\n",
        sequence, (unsigned long long)s3_opcode_full,
        (unsigned long long)s3_opcode_partial, (unsigned long long)s3_opcode_special);
    // One bounded formatting buffer; keep stack and USB bursts small. The host
    // validates all eight groups and their sum before accepting an interval.
    for (unsigned group = 0; group < 8; ++group) {
        char line[512];
        int used = std::snprintf(line, sizeof(line), "[MAME OPCODE COUNTS] seq=%u group=%u counts=", sequence, group);
        for (unsigned i = 0; i < 32; ++i)
            used += std::snprintf(line + used, sizeof(line) - used, "%s%u", i ? "," : "", s3_opcode_counts[group * 32 + i]);
        std::printf("%s\n", line);
    }
    std::fill(std::begin(s3_opcode_counts), std::end(s3_opcode_counts), 0);
    s3_opcode_full = s3_opcode_partial = s3_opcode_special = 0;
}
}
