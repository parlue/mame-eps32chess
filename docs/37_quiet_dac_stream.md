# Quiet DAC stream-rate profile

Builds 44/45 below predate shared-memory IRAM adoption. To reproduce this
isolated baseline after Build 48, set `BAUERINO_MAME_HOT_MEMORY=0`.

The coarse core diagnostic in Build 41 attributed 13.500% / 14.352% of MM IV/V
middlegame wall time to sound-manager updates. Skipping final stereo conversion
is a separate, still optional experiment. This profile instead addresses the
DAC's discarded source samples, independently of that final-conversion change.

MAME 0.220's DAC template starts a two-input, one-output stream at `48000 * 4`
(192,000 samples per emulated second). The selected MM IV/V drivers connect a
1-bit DAC and constant voltage reference to the speaker. The platform is
headless with no audio output, and the sound manager already uses 11,025 Hz.
ROM output-latch writes still reach the real upstream DAC implementation.

The combined default enables `BAUERINO_MAME_QUIET_DAC=1`, selecting a generated
startup-only overlay; explicit `0` restores the original DAC rate. Its machine
reset notifier uses the upstream `sound_stream::set_sample_rate()` API to
request 11,025 Hz on that one DAC output stream. MAME applies pending changes
through its original sound update, sample-index accounting and resampling code.
It changes no CPU/device clock, emulated cycle, timer deadline, ROM instruction,
output latch or DAC write handler. It lowers the resolution of discarded
analog audio; it does not preserve the original 192-kHz audio waveform.

The callback applies only to `mm4` and `mm5`, only while OSD reports no sound
and no WAV/AVI recording is configured. A fresh audible/recording machine
retains its original stream rate. It checks the exact DAC device type, output
index, old rate and platform sample rate before requesting the change. The
startup log records original/requested rates, actual CPU clock and unchanged
sound-update period. No extra per-access/instruction instrumentation is enabled.

All upstream class headers/layouts remain unchanged. Only the source-local
machine-manager class gains a reset callback and machine pointer; the callback
is owned by the current machine and is destroyed with its notifier list.
The generator checks reversal of its injected statements against the original
startup source and retains relative-include resolution. Native CMake retains
the original stream rate. The combined default is qualified below.

This experiment does not claim a PSRAM saving: the original DAC stream buffers
are initially allocated at 192 kHz, and MAME's resizing code expands capacity
without shrinking it. Its intended benefit is less discarded sample-generation
and resampling work. Starting with smaller buffers is a separate future change.

Build 44 compiled in 113.37 seconds: 5,005,703 application bytes (+1,328 versus
Build 40) and 154,636 static RAM bytes (+8). All 17 selected IRAM entries and
the unchanged 2,816-byte IRAM reservation passed linker verification. Total
additional shared SRAM versus Build 34 is 2,824 bytes, below the 4,096-byte
limit. The interpreter source bodies are byte-identical to Build 40. The binary
contains the quiet-DAC marker and no coarse-core/opcode/final-mix markers.

Flash 33 verified all written segments. Full injected-control frontend/ROM
regression 45 passed, including e7e5/d2d4, memory/SYNC checks, launcher,
Virginia, Bots, Board/Notation and clean exits. Both ROM startup logs reported
192,000 -> 11,025 requested DAC samples/s, CPU clock 4,915,200 Hz and sound-update
period 20,000,000,000,000,000 attoseconds, unchanged from pre-device startup.
Both counter-free 180-wall-second middlegame searches passed:

| Model / case | Previous IRAM baseline | Quiet DAC | Relative gain |
|---|---:|---:|---:|
| MM IV / Spanish centre | 26.525% | 30.190% | +13.819% |
| MM V / Dragon attack | 28.012% | 32.106% | +14.616% |

Both use the same 32-ply histories, level `le 9`, original CPU clocks and
unthrottled emulation. Each has 34 retained windows, covering 170.845 /
171.027 seconds; neither completed a bestmove during the search window. Both
passed the clock/timer contract and exited with intact heaps. Minimum internal
free heap was 23,732 bytes in each search, stack 7,456 / 7,372 bytes and PSRAM
2,661,856 / 2,660,112 bytes. CPU wall medians were 78.9 / 78.0%, including
preemption. These are single matched runs per model, not guarantees for every
ROM/position. The 50%-of-original target is still unmet.

The pure final-conversion profile and core diagnostic are disabled for this
isolated comparison against the previously adopted IRAM baseline.
`--expect-quiet-dac` in the benchmark verifies the requested DAC rate, original
4,915,200-Hz CPU clock and sound-update period against the pre-device log before
replay, and records that verification in its evidence.

This profile is adopted. Default Build 45 succeeded in 56.26 seconds with all
runtime-profile environment flags removed. Its complete application is
byte-identical to tested Build 44, SHA-256
`4881bb4e3bc3e4f89be6a3ca6c1c32c174adabd11b2e95ed122070154075747c`.
Flash 33, regression 45 and both sustained searches therefore qualify the exact
default application already running on the Waveshare. No duplicate flash or
repeat test is needed. Physical finger/phone acceptance was not repeated.
Private checkpoints retain exact sources, images and diagnostic/comparison
evidence; no ROMs or binary images are published.
