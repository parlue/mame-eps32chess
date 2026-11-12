# Exclusive core timing and unused final audio conversion

The adopted baseline is the exact Build 38/40 IRAM application: MM IV 26.525%
and MM V 28.012% original speed in the established 32-ply middlegames. The
bounded inline-access experiment remains disabled.
To reproduce Builds 41/42 after the later quiet-DAC/shared-memory adoption,
explicitly set `BAUERINO_MAME_QUIET_DAC=0` and `BAUERINO_MAME_HOT_MEMORY=0`;
otherwise diagnostics inherit the current default.

## Diagnostic profile

`BAUERINO_MAME_CORE_PROFILE=1` instruments function boundaries for scheduler
timeslices, timer execution, the 6502 execution entry, video frame updates,
input-port frame updates, sound-manager updates and the platform OSD update.
It retains the adopted IRAM CPU profile. Diagnostic overlays leave original
class headers/layouts and function bodies intact apart from RAII scopes and
an OSD-init reset. No per-instruction or per-memory-access counters are added.

Scopes subtract their measured children, so CPU, timer callbacks, video,
input, sound and OSD are not counted twice. At the end of an outermost scope,
each five-second report partitions its elapsed wall interval into the seven
exclusive buckets plus unscoped time. Reports include counts and maximum
nesting depth. Report printing is excluded from the following interval.
Preemption, timer-call overhead and waits remain attributed to the active
scope; these are instrumented wall times, not sampled CPU utilization.

The diagnostic is confined to the single MAME worker and CPU-unity build.
It resets between machines, uses fixed storage and creates no task or heap
allocation. The benchmark requires `--expect-core-profile` and labels results
`instrumented-core-profile`; normal speed runs reject the diagnostic.
`tools/report_core_profile.py` checks exact interval sums, expected buckets,
duplicate sequences, missing intervals and minimum sample coverage. It can
repair only the known complete LIVE-line insertion; unknown corruption fails.

Six report tests passed. A C++ test with a deterministic timer independently
checked nested exclusive durations, exception unwinding, reset and interval
rollover. Diagnostic Build 41 compiled in 158.59 seconds: 5,005,799 application
bytes. Flash 31 and full injected-control frontend/ROM regression 43 passed,
including real e7e5/d2d4 replies, memory/SYNC checks and clean exits.
MM IV's Spanish-centre diagnostic search passed with 16 consecutive complete
intervals (80.054 measured seconds), no lost intervals and clean exit. Exclusive
wall shares were CPU 68.520%, sound 13.500%, OSD 8.960%, timer callbacks/handling
3.823%, input ports 2.524%, scheduler 1.783%, video 0.605% and unscoped 0.285%.
Maximum nesting depth was four. MM V's Dragon-attack diagnostic also passed:
16 consecutive complete intervals (80.046 seconds), no lost intervals and a
clean exit. Its shares were CPU 67.507%, sound 14.352%, OSD 9.564%, timers
3.816%, ports 2.528%, scheduler 1.397%, video 0.625%, unscoped 0.212%.
Maximum nesting depth was again four. Both used the original 32-ply histories,
level `le 9` and a 90-wall-second diagnostic search; initial overlap was
excluded. Diagnostic speeds are not candidate speed results.

Host checks can be run with Python (`tools/test_core_profile.py`) and a C++17
compiler. Compile `tools/test_core_profile_accounting.cpp` with
`-Itools/test_stubs` to supply the deterministic timer declaration; compile
`tools/test_discard_mix.cpp` normally. Run both resulting executables with
assertions enabled. The device builds do not include the stub directory.

## Candidate: discard final conversion when no output consumes it

Repeated initial diagnostic intervals attribute approximately 14–15% of wall
time to sound-manager updates. This motivated an opt-in experiment,
`BAUERINO_MAME_DISCARD_MIX=1`, in the sound-manager source only.

The existing platform has no audio output. MAME nevertheless generates speaker
streams, converts the intermediate left/right buffers to a final interleaved
stereo buffer, and hands that buffer to no-op output hooks. The experiment
omits only that final conversion when `m_nosound_mode` is true, no WAV writer
exists and video reports no active recording. The original conversion/output
branch remains verbatim for other cases. The combined platform's audio hooks
and sound hook are no-ops; there is no consumer of `sound_manager::samples()`
in the selected core/platform sources. This optimization is specific to that
output contract; adding such a consumer requires reconsidering the guard.

Speaker-stream generation/mixing, DAC writes, update timers, second-boundary
accounting, stream-buffer maintenance, sample-rate changes and sound-hook calls
remain unchanged. Original ROM clocks and all CPU/device cycles remain intact.
The sample-position remainder advances by the same amount as the old loop,
using one modulo calculation per update. A C++ comparison against the old
loop passed across zero samples, leftover greater than the interval, seven
playback steps and a 10,000-update sequence with changing speeds.

Build 42 compiled in 132.72 seconds: 5,004,543 application bytes, 168 more than
the baseline, and unchanged static RAM. Its selected IRAM code still costs
2,816 shared SRAM bytes versus Build 34, exactly the adopted budget usage;
there is no additional SRAM cost. The generated interpreter bodies were
verified byte-identical to Build 40, and the application contains no opcode
or coarse-core diagnostic markers. Flash 32 and full frontend/ROM regression
44 passed, including both real replies, memory/SYNC checks and clean exits.
Both counter-free 180-second sustained searches passed:

| Model / case | Adopted IRAM baseline | Discard final conversion | Relative difference |
|---|---:|---:|---:|
| MM IV / Spanish centre | 26.525% | 26.590% | +0.246% |
| MM V / Dragon attack | 28.012% | 28.019% | +0.026% |

Each used the same 32-ply history, original clocks, unthrottled emulation and
level `le 9`. There were 34 retained windows per run, covering 170.896 /
171.098 seconds. Neither completed a bestmove during search; both exited with
intact heaps. Minimum internal free heap was 23,740 bytes in each run, stack
7,516 / 7,372 bytes and PSRAM 2,662,020 / 2,661,992 bytes. These single matched
runs establish no meaningful incremental gain. The final-conversion profile
remains opt-in and is not adopted. The separate DAC source-rate investigation
is described in [the following experiment](37_quiet_dac_stream.md).
