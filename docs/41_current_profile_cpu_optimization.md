# Current core profile and CPU-only compiler comparison

Reference: adopted direct-page Build 52/default 53, with original ROM clocks,
quiet DAC and all 21 selected CPU/memory methods in IRAM. The established
counter-free 32-ply middlegames reached MM IV 33.50874% / MM V 35.82792% original
speed. Each uses original clocks, unthrottled emulation, `le 9` and a 180-wall-
second measured search after replay. The exact qualified reference application
is `e2e0b9cb03fd82b8d8a1b7ea7e18024af3c84d1f90bbc49e1cbc3a4d53a86cd5`.

## Current exclusive-time diagnostic

Build 54 enables the existing `BAUERINO_MAME_CORE_PROFILE=1` on that current
default. Direct pages, bank/post-load invalidation, quiet DAC, original clocks,
timers and CPU `-O2` remain enabled. The existing nested scopes subtract measured
children, partitioning each interval without double counting. No instruction
or memory-access counters are added. These are instrumented wall durations,
including task preemption and scope overhead; OSD includes bridge/display work
and its normal yield. They are not pure CPU utilization or candidate speeds.

Build 54 succeeded in 451.46 seconds: 5,008,407 application bytes, 154,748 static
data/BSS and 3,192 total extra shared-SRAM bytes against Build 34. All selected
IRAM placements and the existing 4,096-byte budget passed. Flash 41 verified all
segments. Full injected-control regression 54 passed, including both real ROM
replies, memory/SYNC/bank checks, original frontend and clean exits.

MM IV's Spanish-centre diagnostic retained 16 consecutive complete intervals,
80.026502 seconds, with no missing intervals and maximum nesting depth four:

| Exclusive scope | MM IV wall share | MM V wall share |
|---|---:|---:|
| CPU execution | 75.932% | 75.056% |
| Platform/OSD | 11.291% | 12.246% |
| Timers and timer callbacks | 4.736% | 4.778% |
| Input ports | 3.245% | 3.479% |
| Scheduler | 2.290% | 1.846% |
| Sound manager | 1.482% | 1.590% |
| Video | 0.712% | 0.772% |
| Unscoped remainder | 0.313% | 0.233% |

The requested diagnostic search lasts 90 wall seconds; initial overlap is
excluded. Its instrumented speed is not used as a performance reference.
Audio is now a small part of this workload; CPU work is the first candidate.
MM V's Dragon-attack diagnostic also passed: 16 consecutive complete intervals,
80.032110 seconds, no missing intervals, maximum depth four and clean exit.
Both retained direct pages without invalidation during replay/search, their
quiet-DAC contract and original ROM clocks/timer periods. No complete interval
was reconstructed beyond the existing known LIVE-line repair. CPU execution
dominates in both ROMs, motivating the CPU-only compiler comparison below.

## Candidate: CPU-only `-O3`

`BAUERINO_MAME_CPU_OPT=O3` changes only the existing CPU-source compilation
profile from `-O2` to `-O3`. All other MAME/SDK profiles remain unchanged.
The default stays `O2` until qualification. Only the exact values `O2` and `O3`
are accepted; fast-math/`Ofast` and arbitrary compiler option strings are rejected.
Signed-char semantics, RTTI/exceptions, jump-table/switch flags and the mandatory
`-fno-merge-constants` delegate-identity fix remain in force. No interpreter
instruction bodies, CPU clocks, timers or Lua code are rewritten. The coarse
diagnostic is removed for candidate timing. Ten configuration tests pass.

The actual linker SRAM reservation must still fit the original 4,096-byte
growth bound, including any compiler-induced IRAM/literal expansion. Code size
and device correctness must pass before matching counter-free speed runs.
Larger optimization settings are not assumed to be faster on this device.

Build 55 succeeded in 594.89 seconds. Application accounting is 5,016,567 bytes,
9,596 bytes above Build 53; static data/BSS stays 154,644. Actual total shared
SRAM growth is 3,344 bytes against Build 34, 256 bytes above the reference.
IRAM reservation is 3,328 bytes above Build 34, within the same 4,096-byte bound.
All selected methods remain in IRAM. The application contains no core/opcode
diagnostic or old inline/metadata experiment markers. Full image SHA-256:
`c1f757dc84ffc9683cee7d2bab694fe22a88ceca2cf1c3cf57edff28741e93a0`.

Flash 42 verified all segments. Full injected-control regression 55 passed,
including e7e5/d2d4, all memory/SYNC/bank tests, post-load registration,
launcher/Virginia/Bots/gestures and clean exits. Both subsequent uninstrumented
180-wall-second searches completed after the unchanged 32-ply replay:

| Model / case | Qualified O2 reference | CPU-only O3 | Relative difference |
|---|---:|---:|---:|
| MM IV / Spanish centre | 33.50874% | 33.64835% | +0.417% |
| MM V / Dragon attack | 35.82792% | 36.25524% | +1.193% |

Each retained 29 intact rate intervals, covering 145.707 / 145.510 seconds.
Known USB output interleaving reduces intact rate coverage; partial rate lines
were not reconstructed. Neither completed a bestmove in the search window.
Original CPU clocks, timer period, quiet-DAC contract, direct-page validity and
clean exits passed. Minimum internal heap was 23,360 bytes for both, stack
7,516 / 7,372 bytes and PSRAM 2,663,720 / 2,663,896. CPU wall medians were 76.3 /
75.3%, including preemption. These small differences require a fresh reference
before making a disposition; the diagnostic speeds above are not used here.

Unmodified-default Build 56 succeeded in 411.58 seconds with the explicit CPU
optimization flag removed. Its complete application is byte-identical to the
qualified O2 Build 53, hash `e2e0b9cb03fd82b8d8a1b7ea7e18024af3c84d1f90bbc49e1cbc3a4d53a86cd5`.
It restores the original 5,006,971 application bytes and 3,088-byte SRAM growth.
Flash 43 restored that exact standard; full regression 56 passed again before
the fresh MM-IV control. The source default remains `O2`; `O3` is still opt-in.

## Fresh reference and disposition

The fresh exact-image O2 MM-IV control completed its 180-wall-second search
with 33 retained intervals, 165.577 seconds, original clocks/timers, direct-page
and quiet-DAC checks, no bestmove in the window and clean exit. It achieved
33.62484% original speed. Relative to this fresh control, O3's 33.64835% is only
**+0.070%**. The fresh reference is itself 0.346% above the earlier 33.50874%
reference, accounting for most of the initially apparent +0.417% difference.
No fresh MM-V reference was run; its +1.193% above the earlier reference remains
a limited single comparison, not proof of an equally reproducible improvement.

O3 is not adopted as the default. The measured results do not establish a
compelling, consistent benefit across these two ROMs for the additional 9,596
application bytes and 256 shared-SRAM bytes. This does not claim O3 is slower
in all workloads or prohibit the explicit opt-in for future comparisons.

The device remains on the exact qualified O2 application restored by Flash 43.
Its full frontend/ROM regression 56 and this fresh sustained control passed.
The build directory likewise contains the byte-identical O2 default. Original
CPU clocks, timers, IRAM routines, direct pages and quiet DAC are retained.
Current CPU-only configuration accepts `BAUERINO_MAME_CPU_OPT=O2|O3`; the default
is O2, and all ten portable configuration tests pass. No new physical finger
or phone acceptance is claimed; device controls were injected over USB.
