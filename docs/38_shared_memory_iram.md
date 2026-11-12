# Shared memory entry points in IRAM

Historical qualification of Builds 46–48. With the later direct-page default,
set `BAUERINO_MAME_FLAT_PAGES=0` to select the bank-base-per-access path measured
here. This flag is independent of the retained shared-method IRAM placement.

The immediate reference is adopted Build 44/45, including quiet DAC audio:
MM IV 30.190% and MM V 32.106% original speed in the established 32-ply
middlegames. Original CPU clocks and timer periods remain the reference.

The qualified combined default enables `BAUERINO_MAME_HOT_MEMORY=1`; explicit
`0` restores the four original flash methods. This placement profile retains
the 17 existing IRAM CPU routines and additionally places the paged interface's
four virtual methods in IRAM: data read, data write, operand read and opcode
read. Their previous linked function text totals 348 bytes. These shared entry
points are reached across instruction bodies, rather than expanding memory
operations inline in selected instructions.

The generator only adds `IRAM_ATTR` and `noinline` to definitions of the four
methods in the source-local paged-interface class. Exact reversal checks and
comparison against Build 45 establish that the CPU/interpreter bodies, memory
checks, interface dispatch, page tables, bank handling and fallback remain
unchanged. Upstream headers and class layouts are untouched. The disabled
generator was checked in memory against the exact Build-45 source without
mutating build inputs.

The combined builder requires the existing IRAM profile and CPU unity. The
separate direct/inline-memory, opcode histogram and memory-probe flags remain
mutually exclusive in the CPU generator. Quiet DAC retains its adopted
default; final stereo conversion remains original. No new live counters or
per-access mode checks are introduced.

`tools/verify_hot_routines.py --hot-memory` requires all four actual entry
addresses in internal instruction RAM, as well as all 17 original selected
routines. With the flag omitted, it requires the four entries outside IRAM.
The existing 4,096-byte total shared-SRAM limit against Build 34 is unchanged;
it includes literal/alignment reservation and static data/BSS. Diagnostic
opcode/core profiles are rejected as speed candidates.

Build 46 succeeded in 127.43 seconds: 5,005,711 application bytes (+8 versus
Build 45) and unchanged 154,636 static RAM bytes in PlatformIO's accounting.
Actual shared SRAM cost versus Build 34 is 3,336 bytes, including 3,328 IRAM
reservation and eight data bytes from the adopted DAC profile. This is 512
bytes more than Build 45 and remains within the unchanged 4,096-byte limit.
All four actual methods and all original 17 CPU entries passed IRAM placement
verification. The four method text sizes remain 85/89/85/91 bytes.

Flash 34 verified all written segments. Full injected-control regression 46
passed: e7e5/d2d4, memory/SYNC checks, launcher/Virginia/Bots/gestures,
Board/Notation, clean ROM exits and return to the original frontend. Both
ROMs retained the 4,915,200-Hz CPU clock and original sound-update period.
Both matched counter-free 180-wall-second searches passed, including the
quiet-DAC clock/timer contract and clean exits:

| Model / case | Previous Build 44/45 | Candidate 46 | Relative difference |
|---|---:|---:|---:|
| MM IV / Spanish centre | 30.190% | 30.789% | +1.985% |
| MM V / Dragon attack | 32.106% | 32.684% | +1.799% |

They use the same 32-ply histories, original clocks, unthrottled emulation and
level `le 9`. MM IV retained 34 rate windows (171.108 seconds), MM V 32
(160.820 seconds); the existing harness can lose individual rate lines through
USB log interleaving. Neither completed a bestmove within the search window.
Minimum internal free heap was 23,220 bytes in each search, stack 7,600 / 7,456
bytes and PSRAM 2,661,940 / 2,661,908 bytes. CPU wall medians were 78.4 / 77.8%,
including preemption.

Because these differences are small, a fresh standard control is required
before adoption. Build 47 succeeded in 124.22 seconds and its application is
byte-identical to Build 45. Disabled-profile linker verification restores the
2,824-byte total shared-SRAM cost, with the four memory methods in flash.
Flash 35 and full regression 47 restored this standard for fresh searches in
reversed model order (MM V then MM IV). Both 180-second controls passed with
34 retained windows, unchanged clock/timer contracts and clean exits:

| Model | Fresh standard 47 | Candidate 46 | Gain versus fresh control |
|---|---:|---:|---:|
| MM IV | 30.200% | 30.789% | +1.951% |
| MM V | 32.148% | 32.684% | +1.667% |

Fresh baseline changes versus the original reference were only +0.033% /
+0.130%; the small candidate advantage remains against the faster baseline
for each ROM. Measured control windows cover 170.841 / 170.957 seconds.
These are limited matched runs with a repeated baseline, not a statistical
guarantee or a prediction for other ROMs/positions. Placement and the noinline
compiler decision are measured together; there is no noinline-only control.
The 50%-of-original target remains unmet.

The profile is adopted. Default Build 48 compiled in 123.79 seconds with all
runtime-profile environment flags removed. Its complete application is
byte-identical to qualified Build 46, SHA-256
`ee88552a89344defecc154de46238064d2bf28abefd7d3c1ac74b6175ac71971`.
Flash 36 verified all written segments and final injected-control regression
48 passed again, including both real ROM replies, memory/SYNC tests, original
frontend flows and clean exits. The Waveshare now runs the adopted default.
The current defaults retain quiet DAC
and the 17 original IRAM routines. `BAUERINO_MAME_HOT_MEMORY=0` disables only
the four added placements; `BAUERINO_MAME_HOT_ROUTINES=0` disables both groups.
Explicit old direct/inline/probe CPU profiles do not silently inherit this
new placement. Coarse-core diagnostics intentionally inherit the current
default. Seven portable build-configuration tests verify these interactions.

To reproduce a pre-Build-48 baseline, explicitly disable `HOT_MEMORY` (and
disable `QUIET_DAC` as well for baselines predating its adoption). Original
source headers/layouts, CPU/device clocks and timer periods remain unchanged.
Physical finger/phone acceptance has not been repeated. Private checkpoints
retain exact images, sources and all original/candidate/control evidence.
