# Qualified direct CPU memory experiment

Build 32 compares direct qualified accesses against normal Build 31 (the same
CPU implementation and sustained-search baseline as Build 29).

Enable the experiment with `BAUERINO_MAME_DIRECT_MEMORY=1` when invoking the
combined Bauerino PlatformIO build. It requires CPU unity compilation and is
off by default. Do not combine it with the startup memory microbenchmark.

`prepare_direct_memory.py` generates private copies of the two instruction
implementation files. It replaces only read/write call prefixes and verifies
an exact reverse transformation against the original files. PC increments,
dummy reads/writes, cycle accounting, instruction arithmetic and interrupt
logic remain unchanged. The two opcode prefetch calls use the same adapter.
Upstream class headers and layouts are unchanged in every translation unit.

The adapter borrows the qualified page table and its three fallback caches.
It checks the exact memory-interface identity on every access. Other CPU
interfaces retain their virtual operations. The paged interface registers the
context only after the startup memory self-test and fresh page-table creation;
its destructor clears the context before destroying its resources. The
current application has one MAME execution worker. Concurrent MAME workers
would require independent contexts and are not supported by this experiment.

All direct accesses still use `s3_fast_memory`: only qualified plain RAM/ROM
or real bank pages qualify, bank bases are re-read, map changes invalidate
the table, and device/tap/unqualified accesses use MAME's caches. Memory mode
0/1 remains on the original interface path. No original emulated clock or
frontend timing is changed.

Build 32 compiled in 146.73 seconds: 5,040,871 app bytes and 154,644 static
RAM bytes. Flash 24 and frontend/ROM regression 36 passed, including real
MM IV e7e5 and MM V d2d4 replies, startup memory/SYNC self-tests, launcher,
Virginia, Bots, gestures, Board/Notation and clean task exits.

Its sustained MM IV Spanish-centre search regressed to about 20.1% original
speed against 24.8% for Build 29. This candidate will not become the default.
The compiler expanded some adapter/cache paths into instruction bodies;
CPU-related text symbols grew substantially. This is a plausible instruction
cache pressure explanation, not a measured cache-miss attribution.

MM V's captured 180.297-second Dragon search also regressed, with 34 retained
windows averaging 20.909% against the previous 26.228%. Its engine exit was
clean, but the host harness timed out because the quit acknowledgement was
embedded in an interleaved profile line. This is partial timing evidence,
not a fully passing benchmark. The harness now matches complete acknowledgement
tokens within a line, including explicit rejected responses. The real failure
line and accepted/rejected/token-boundary cases were checked independently.

Build 33 marks the four adapters noinline, retaining shared code and the same
memory semantics. It compiled in 178.92 seconds: 5,000,811 application bytes
and 154,644 static RAM bytes, 3,332 fewer application bytes than stable Build
31. Flash 25 and frontend/ROM regression 37 passed, including both expected
real ROM replies and clean exits. Both sustained searches passed all harness
checks with 34 retained intervals each:

| Model / position | Stable Build 29 | Shared adapter Build 33 | Relative change |
|---|---:|---:|---:|
| MM IV / Spanish centre | 24.773% | 24.291% | -1.945% |
| MM V / Dragon attack | 26.228% | 25.700% | -2.013% |

Each search used the same 32-ply history, level le 9, unthrottled emulation and
180 wall seconds as its baseline. Replay was excluded. Retained windows cover
171.069 / 171.188 seconds. Neither search finished a bestmove in its window.
Both exited with intact heaps. Minimum free stack was 7,372 / 7,600 bytes;
minimum internal heap 26,528 / 26,540 bytes. CPU wall medians were 71.4 / 70.3%.

These are single matched runs per model, not a statistical confidence claim
for a precise 2% regression. They provide no clear speed benefit to justify
adopting the adapter. The substantial inline regression also demonstrates
why isolated access microbenchmarks do not establish whole-interpreter gains.
The ownership check, shared-call overhead and changed code placement remain
possible factors; no individual cause has been isolated.

Direct memory remains opt-in. Normal Build 34 succeeded in 133.26 seconds,
restoring 5,004,143 application bytes and 154,628 static RAM bytes. Its entire
application binary is byte-identical to stable Build 31, with SHA-256
`4cd4dae17bf4ef02d6c00dd56d5abd97f0821a194e7e2cc29d03995c4e188b96`.
The original CPU source identity was independently checked with the option
disabled. Flash 26 verified all written segments. Final frontend/ROM regression
38 passed, including both real ROM replies, memory/SYNC self-tests, launcher,
Virginia, Bots, gestures, Board/Notation and clean exits with intact heaps.
The binary contains no direct-memory marker. No extra stable performance run
is needed for this identical binary. This is an injected-control regression,
not a new physical finger/phone acceptance test.

The generator also resolves relative output paths before constructing absolute
upstream includes. Both relative and absolute invocations were checked in
memory against all generated Build-33 sources without modifying build inputs.

Private checkpoints preserve exact binaries, generated sources and test
evidence for Builds 32, 33 and 34. No ROMs or binaries are published.
