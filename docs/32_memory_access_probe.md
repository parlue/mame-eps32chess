# Bounded startup memory-path probe

Reference: Build 29, MM IV 24.77% / MM V 26.23% sustained middlegame speed.
CPU execution accounted for approximately 70% of wall time after selection
polling was reduced. This probe compares access paths without per-access
instrumentation in the live interpreter.

The combined builder opts in only with environment variable
`BAUERINO_MAME_MEMORY_PROBE=1`. Without it, the generated CPU source is exactly
the Build-29 source. Native CMake keeps the default disabled. A const-pointer
probe accessor exposes only pages already qualified by the fast-memory layer;
it adds no state/layout or live access counters. The probe runs after the
existing read/write/invalidation self-test and fresh page-table construction,
before CPU execution. It makes no writes and does not advance emulated time.

Each ROM uses 256 predetermined bytes spread over all eligible full RAM/ROM
pages. One order is address-sorted, the other deterministically shuffled.
Neither pattern represents measured instruction traffic. Opcode/data spaces
must be shared, as they are in the two current models. Device I/O, taps,
ineligible pages and separate opcode maps are not probed.

Paths: raw const byte pointer; upstream data cache; qualified page helper;
virtual data, operand and opcode reads. All use the same extra harness
function-pointer call and non-IPA thunks. The virtual/direct difference also
includes interface-owned pointer loads, not solely virtual-dispatch cost.
Actual interpreter compilation may optimize differently; assembly inspection
is needed before applying conclusions to a hot-path change.

There are three order-rotated repetitions per method/pattern and 16,384 reads
per repetition. Every path is warmed and checksummed against the same original
cache bytes. Only 256-read blocks are timed, with watchdog/idle yielding outside
those blocks. Timer overhead and task preemption remain in reported wall
timings. Results are comparative microbenchmarks, not percentages of search
time, emulated CPU cycles, or projected whole-engine speedups.

`tools/report_memory_probe.py` requires both models, all 72 timing records,
exact repetitions, equal checksums and successful probe markers. Raw logs and
firmware remain private. Default-off source identity was verified before the
diagnostic build. Build 29 is retained for rollback.

## Device results

Diagnostic Build 30 succeeded in 149.02 seconds, with 5,006,371 application
bytes and 154,628 static RAM bytes. Flash 22 and the full frontend/ROM
regression passed. Both models produced all 72 timing records, with matching
checksums and intact heaps on exit. Rounded median nanoseconds per read,
including the common harness, were:

| Path | MM IV ordered / shuffled | MM V ordered / shuffled |
|---|---:|---:|
| Raw pointer | 74.4 / 73.9 | 74.2 / 74.2 |
| Upstream cache | 264.6 / 367.3 | 263.5 / 381.0 |
| Qualified pages | 167.0 / 168.0 | 167.8 / 168.4 |
| Virtual data | 227.1 / 227.2 | 224.3 / 224.4 |
| Virtual operand | 229.3 / 228.9 | 226.9 / 226.3 |
| Virtual opcode | 234.6 / 235.5 | 231.6 / 231.6 |

These results support testing a more direct qualified-memory path at the CPU
entry points. They do not predict an equal whole-engine gain: instruction
traffic proportions, device I/O, arithmetic, dispatch and scheduler time are
not measured by these artificial read patterns. No CPU optimization is enabled
by this experiment.

The local objdump attempts did not yield usable instruction decoding. Instead,
Build 31 saved the compiler's own assembly using the optional host-only flag
`BAUERINO_MAME_CPU_ASM=1`. The actual `m6502_device::prefetch()` body loads the
memory-interface vtable, loads its read-sync entry at offset 16 and executes
`callx8`. Thus the indirect fetch call remains in this interpreter build;
the microbenchmark is not the only evidence for its existence.

Normal Build 31 succeeded in 124.63 seconds, restoring exactly 5,004,143 app
bytes and 154,628 static RAM bytes, equal to Build 29. With the startup probe
disabled, its generated CPU source is byte-identical to Build 29 and the
firmware binary contains no probe marker strings. Flash 23 and the final
frontend/ROM regression passed, including real e7e5/d2d4 replies, memory/SYNC
checks, Virginia/Bots/gestures, Board/Notation and intact heaps on both exits.
No new sustained-search speed gain is claimed. Build-29 middlegame figures
remain the latest performance measurements.

Private checkpoints: `combined-build30-memory-probe` and
`combined-build31-probe-off`, with exact images, sources, timing evidence and
compiler assembly. No ROMs or binary artifacts are distributed.
