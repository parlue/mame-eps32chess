# Independent 6502 data lookup cache

Baseline: stable Build 20, source commit
`827df51ecbce11b60eef8a31008c443075bf9c79`, with CPU-only O2/jump tables.
Its repeated short ROM tests reported MM IV 18.92-18.93% and MM V 20.26-20.27%
of original speed. This experiment keeps that compiler profile unchanged.

## Scope

The previous optional cached interface shared the operand cache for ordinary
data accesses too. Alternating instruction bytes in ROM and data in RAM can
replace each other's cached handler range. This variant allocates a third,
independent upstream `memory_access_cache<0, 0, ENDIANNESS_LITTLE>` for ordinary
reads/writes. Opcodes and operands keep their two original caches.

The third cache is created after upstream CPU `init()` completes and before
reset/execution begins. Upstream init only registers/initializes state and
access caches; it does not execute instructions. Its unique_ptr owns the extra
cache and removes its address-space change notifier when destroyed. The two
original caches retain the prior owning destructor. A partial-start failure
still has owning pointers for all successfully allocated caches.

This caches handler lookup, not RAM contents. Reads/writes still call the
selected upstream handler; I/O, taps, address masks and invalidation when the
map changes remain upstream behavior. No direct RAM/ROM pointer bypass,
instruction change, clock change, CPU-header/layout change or memory-map edit
is involved. The CPU and caches use ordinary PSRAM allocation. CPU profile
timing and watchdog yielding are unchanged.

USB diagnostic `mame-cache 0|1` selects original data dispatch versus this
independent data cache before loading a ROM. `mame-speed` bit 0 has the same
meaning; bit 1 retains the separate optional internal-CPU experiment. Selection
while the worker is busy is rejected. The candidate defaults to mode 1, so a
default-start regression can verify the actual proposed shipping setting.
It must only replace the stable baseline if comparison and repeat tests pass.

`[MAME DATA CACHE]` records separate data/operand/opcode pointers and the
44-byte cache object size. That is not the total allocation cost: the owning
interface and upstream map-notifier bookkeeping also use memory.

## Validation

Build 21 succeeded in 381.12 seconds: 5,009,355 application bytes and 154,604
static internal RAM bytes. The insignificant -20/-8 byte differences from
Build 20 are not presented as a size optimization. Assets, partition layout,
ROM filesystem and compiler profile are unchanged. The source audit confirms
all CPU source outside `device_start` and the default-mode constant is identical
to Build 20. Native CMake already calls the same generator; it was not separately
device-tested. Flash 15 verified every segment.

| ROM | Build 20 stable | Build 21 cache off | Cache on | Default after reset |
|---|---:|---:|---:|---:|
| MM IV | 18.92-18.93% | 18.75% | 19.98% | 19.88% |
| MM V | 20.26-20.27% | 20.26% | 21.39% | 21.39% |

These are MAME's average percentages of emulated time versus wall time for
the same short ROM/input sequence. Same-binary improvement is 6.6% / 5.6%;
against the prior stable Build 20, the repeated cache results improve speed by
approximately 5-6%. Both comparisons are shown to avoid selecting the most
favorable baseline. This is not a long search benchmark or full-game timing
validation. Clock, interrupts, ROM and UCI options are unchanged.

All three complete regressions passed: actual MM IV e2e4/e7e5 and MM V d2d4,
six result-0 exits with heap integrity, return to Virginia, launcher/gestures,
Bots/all 215 portraits, UCI Options and board/notation. The cached runs each
logged two sets of three distinct data/operand/opcode cache addresses, with
44-byte data caches. The reference had no extra data-cache log. Internal heap
remained approximately 26 KiB; minimum observed worker-stack reserve was 7,324
bytes. Post-exit PSRAM was:

| Run | After MM IV | After MM V |
|---|---:|---:|
| Cache off | 4,819,000 | 4,818,956 |
| Cache on | 4,819,012 | 4,818,948 |
| Default after reset | 4,819,000 | 4,818,956 |

No retained per-load cache allocation is evident in these checks; this is not
an unbounded reload stress test. New screenshots, physical finger input and
phone access were not independently reverified for this core-only change.

The source defaults to mode 1, which is the tested shipping setting. Exact
images, source, source audit and all three device logs are saved under
`backups/takeover-20261110/combined-build21-data-cache`. Build 20 is retained
unchanged as rollback.

## Next performance target

At least 50% original speed is the user's next practical threshold. The current
19.9-21.4% is a reproducible intermediate result, still approximately 2.3-2.5x
away. The next investigation should measure/reduce per-access handler overhead,
while preserving actual device accesses, mapping changes and interrupts. No
claim is made that the remaining factor is guaranteed or already implemented.
