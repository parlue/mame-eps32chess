# Opcode frequencies and bounded IRAM experiment

The direct-memory experiment did not provide a sustained gain. This step
first measures the instruction-dispatch mix, then tests a limited placement
change in the interpreter itself. No emulated clock or instruction semantics
are changed.

`BAUERINO_MAME_OPCODE_PROFILE=1` enables diagnostic Build 35. Full-dispatch
starts are counted by instruction state, while partial resume calls and
special states are counted separately. An interrupted instruction is not
counted as a fresh instruction when resumed. BRK entries can include interrupt
dispatch, so these are routine frequencies rather than decoded ROM-only
instruction counts. Counts do not measure time spent in each routine.

The older `[MAME CPU] cycles=` field is an accumulated initial-minus-remaining
budget delta, not a verified retired-cycle counter: `abort_timeslice()` in
`diexec.cpp` can remove remaining budget and track it as stolen cycles. Do not
derive original CPU speed or average instruction cycles from that field.
Machine-time/wall-time rate windows and diagnostic dispatch-start counts are
independent of it. In particular, this experiment retains every original
per-cycle budget check; it does not assume a memory callback cannot reschedule
the CPU within an instruction.

Eight bounded records per reporting interval cover all 256 opcode bins. The
host validates sequence/group uniqueness, complete records and the full-start
sum. Only intervals inside the search phase, excluding its first ten seconds,
are aggregated. Corrupt interior intervals fail validation. Diagnostic searches
use the existing 32-ply Spanish-centre/Dragon histories and level le 9, with
90 wall seconds per model. Their instrumented speed is not compared to normal
firmware. Baseline speed tests remain the original 180-second runs.

The captured USB stream can contain a complete frontend LIVE message inserted
inside a decimal count. Only this recognized insertion is rejoined, including
split headers; all count totals must still match. Entirely missing records
require explicit `--allow-missing-intervals`: the complete interval is excluded
and listed in the result, with at least eight intervals and 80% completeness
required. Sequence gaps also count as loss. Truncated values, duplicate groups
and incorrect sums still fail; no missing count is reconstructed or estimated.

`BAUERINO_MAME_HOT_ROUTINES=1` selects a separate, counter-free experiment.
The explicit routine list is in `tools/65xx_hot_routines.json`. The generator
adds only IRAM placement/noinline attributes to selected definitions and checks
an exact body round trip. Upstream class layouts remain unchanged. The profile,
hot-routine and previous memory experiments are mutually exclusive; normal
CPU generation remains unchanged when they are disabled. After qualification,
the combined unity build enables hot routines by default. Set
`BAUERINO_MAME_HOT_ROUTINES=0` to reproduce the former CPU profile. Explicit
diagnostic modes automatically default hot routines off; explicitly conflicting
flags are rejected. Native skeleton and non-unity builds retain their previous
default CPU profile.

`verify_hot_routines.py` requires every selected function symbol to be in
internal instruction RAM. It compares the actual linker shared-SRAM reservation
against the normal map and rejects increases above 4 KiB before flashing.
This includes literals and alignment, not merely the sum of function text.
The size lookup includes GCC's outlined `$part$` bodies as well as normal
function symbols. Selected functions and any such parts must all reside in
IRAM. This avoids mistaking an inlined wrapper for the whole routine.
Runtime heap, stack, real ROM replies, GUI/clean exits and both sustained
180-second searches must still be checked on the device before adoption.

Build 35's frontend/ROM regression passed. MM IV yielded 26,379,339 validated
instruction starts; one interval (77) was explicitly excluded for a completely
missing count group. Its most frequent routines were BMI (12.94%), BEQ (8.23%),
LDA absolute-X (5.63%), INY (5.59%) and CMP zero-page (5.14%). These are frequencies,
not their shares of wall time.

Build 37 selects the first 16 frequent MM-IV instruction routines within a
3-KiB estimated text budget, plus prefetch. They cover 71.96% of the validated
MM-IV instruction starts. Build 36 was a compile-only size study, never flashed;
it exposed the need to include outlined bodies in the selection.

MM V yielded 29,818,302 validated instruction starts across 15 complete
intervals, with no excluded intervals. The same selected routines cover 70.79%
of its starts. Partial resume calls were 7,036 / 7,886 for MM IV / MM V.

Build 37 compiled in 133.50 seconds, with 5,004,375 application bytes and
154,628 data/BSS bytes. All 17 selected symbols are in IRAM; no counters or
direct-memory experiment marker remains in the binary. Actual extra shared
SRAM reservation is 2,816 bytes, within the 4,096-byte limit. The normal PIO
data/BSS number does not include this instruction-memory cost.

Flash 28 and full frontend/ROM regression 40 passed, including MM-IV e7e5 and
MM-V d2d4 replies, startup memory/SYNC checks, Virginia/Bots/gestures,
Board/Notation and clean exits. Both counter-free sustained searches passed:

| Model / case | Previous Build 29 | IRAM Build 37 | Relative gain |
|---|---:|---:|---:|
| MM IV / Spanish centre | 24.773% | 26.525% | +7.072% |
| MM V / Dragon attack | 26.228% | 28.012% | +6.800% |

Both used the same 32-ply histories, original clocks, unthrottled emulation,
level le 9 and 180 wall seconds of search. The 34 retained intervals per run
cover 170.932 / 171.081 seconds, excluding replay and initial overlap.
No bestmove completed within either search window. Both engines exited with
intact heaps. Minimum free internal heap was 23,740 bytes in each search;
minimum stack 7,456 bytes. Minimum PSRAM was 2,662,056 / 2,659,788 bytes.
CPU wall medians were 69.3 / 68.0%, including preemption. These are single
matched runs per model, not guarantees for every ROM or game position.

This bounded profile is adopted for the combined firmware. The 50%-of-original
speed target remains unmet. Default Build 38 succeeded in 51.91 seconds with
all runtime-profile selection environment flags removed. Its complete application binary
is byte-identical to tested Build 37 (SHA-256
`1688997b5b17f72ee820710dddc06741bfcef21578a5f709e7657f9e476bfd5e`).
Thus Flash 28, regression 40 and both sustained runs validate the actual
default application already running on the device; no duplicate flash or
test run is required. Explicit hot-profile-off CPU generation was separately
verified byte-identical to the former stable source.

Six host tests in `tools/test_opcode_profile.py` cover inherited CMOS mapping,
known full/partial counts, a LIVE insertion inside a number, explicit group
loss, whole-interval gaps, incorrect sums and the 80% completeness threshold.
Python syntax checks also passed. Physical finger/phone acceptance was not
repeated; the device tests use injected controls.

Private checkpoints preserve diagnostic Build 35, compile-only Build 36 and
qualified Builds 37/38 with exact sources, images and evidence. No ROMs or
binary images are published.

IRAM placement and the associated noinline compiler decision are tested as
one profile. There is no separate noinline-only control or hardware cache-miss
counter measurement, so any resulting gain cannot be attributed exclusively
to instruction-cache misses.
