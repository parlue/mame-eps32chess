# Bounded inline memory access inside IRAM routines

Build 39 combines the qualified RAM/ROM adapter with the adopted IRAM profile.
It is opt-in through `BAUERINO_MAME_HOT_DIRECT=1`; the default remains Build
38's IRAM profile. The measured incremental benefit does not justify adopting
the larger variant on the available single matched runs.

The direct-access list is `tools/65xx_hot_direct.json`: prefetch, BMI, BEQ and
CMP zero-page. Only these four bodies have memory call sites rewritten. The
remaining instruction bodies retain their original accesses and the existing
17-function IRAM placement is kept. Generator round trips retain all cycle
guards, PC increments, arithmetic and interrupt handling. Class headers and
layouts are unchanged across translation units.

The inline template matches the exact active memory-interface identity. It
uses the existing qualified page table and the appropriate data/operand/opcode
fallback caches. Bank bases are re-read, map changes invalidate the table, and
I/O/unqualified accesses retain MAME's handlers. The borrowed context is bound
after startup memory self-tests and fresh table construction, then cleared by
the owning paged interface before destruction. Other interfaces fall back to
their virtual methods. As before, there is one MAME execution worker.

The separate whole-interpreter direct-memory experiment remains unchanged in
scope and uses shared noinline adapters. The new bounded profile forces these
adapters inline only at the selected sites. It does not enable diagnostic
instruction counters or alter the original emulated clocks.

The linker check now includes both the IRAM reservation and static data/BSS
growth, measured by the static DRAM end against Build 34. Build 39 uses 3,840
extra IRAM bytes plus 16 data bytes: 3,856 total, below the unchanged 4,096-byte
limit. This is 1,040 bytes more than adopted Build 38. All 17 selected function
entries are in IRAM. The generated default IRAM sources were independently
checked against Build 38 without modifying build inputs.

Build 39 compiled in 130.86 seconds. Guarded Flash 29 and device regression
41 passed: original launcher, Virginia, 215 Bots, injected gestures, real
MM-IV e7e5/MM-V d2d4 replies, memory/tap/SYNC checks, Board/Notation, clean
exits and return to the original frontend. Both counter-free sustained searches
also passed against the immediate adopted Build 37/38 baseline:

| Model / case | IRAM baseline | Bounded inline memory | Relative difference |
|---|---:|---:|---:|
| MM IV / Spanish centre | 26.525% | 26.808% | +1.070% |
| MM V / Dragon attack | 28.012% | 28.349% | +1.202% |

Each run uses the same 32-ply history, original clocks, unthrottled emulation,
level `le 9` and 180 wall seconds of search. Each has 34 retained windows,
covering 171.088 / 171.077 seconds. Neither completed a bestmove within the
search window; both exited with intact heaps. Minimum internal free heap was
22,700 / 22,692 bytes, stack 7,372 / 7,516 bytes and PSRAM 2,659,832 /
2,660,328 bytes. CPU wall medians were 68.8 / 67.6%, including preemption.
These are single matched runs, without a repetition or order reversal to
separate a roughly 1% effect from run-to-run variability. No general speed
improvement is established. The 50%-of-original target remains unmet.

The experiment costs 916 additional application bytes and 1,040 additional
shared SRAM bytes versus Build 38. It remains available for controlled future
experiments, disabled by default. Build 40 compiled in 122.97 seconds with all
runtime profile flags removed; its complete application is byte-identical to
the adopted Build 38 (SHA-256
`1688997b5b17f72ee820710dddc06741bfcef21578a5f709e7657f9e476bfd5e`).
Guarded Flash 30 restored this standard firmware on the Waveshare; all written
segments verified. Device regression 42 passed again, including the frontend,
both real ROM replies and clean exits. The device is back on the adopted IRAM
application, with the bounded inline-memory experiment disabled.

Six existing opcode-report host tests and Python syntax checks passed. No new
physical finger/phone acceptance is claimed. Private checkpoints preserve
both exact applications, generated sources and device evidence. No ROMs or
binary images are published.
