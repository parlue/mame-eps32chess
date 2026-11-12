# Bounded low-page metadata experiment

Historical experiment and restoration of Build 48. With the later direct-page
default, select `BAUERINO_MAME_FLAT_PAGES=0` for its reference path. Explicit
`HOT_METADATA=1` automatically keeps the incompatible direct-page path off.
Device-state statements below describe the end of this rejected experiment.

Reference: adopted Build 48, with shared memory entry points in IRAM and quiet
DAC output. Its matched middlegames achieved 30.789% (MM IV, Spanish centre)
and 32.684% (MM V, Dragon attack) of original speed. This experiment is opt-in
with `BAUERINO_MAME_HOT_METADATA=1`; the qualified default stays unchanged.

The experiment puts copies of the qualified read/write page descriptors for
addresses 0x0000–0x07ff into a single internal DRAM slot. On ESP32-S3 the slot
is 200 bytes, including ownership and validity. It contains no guest RAM or
ROM bytes. Bank descriptors still dereference the current upstream bank base
on every access. Fixed descriptors remain the existing qualified pointers.
The original page whitelist, contiguous-page qualification and normal MAME
fallback remain in force. Separate opcode maps use their original table.

Only one helper owns the slot. Additional helpers fall back to their original
tables; destroying the owner releases the slot. Startup claims the slot only
after notifier installation succeeds. A map-change notification invalidates
the slot alongside the original table. No allocation or rebuilding occurs
inside the callback. This is specific to the existing single MAME worker;
it is not a thread-safe cache for concurrent emulator workers.

The startup self-test compares all qualified bytes with real upstream handlers.
For this experiment it selects a writable byte in the first eight pages for
the saved-byte read/write/restore and temporary read-tap test, so that a stale
internal descriptor cannot bypass the tap unnoticed. The helper is then
destroyed and freshly constructed before ROM execution. Internal placement is
checked on the device. No instruction counters are added.

The benchmark's `--expect-hot-metadata` flag verifies both startup slot claims
(self-test and fresh execution helper), their internal placement and exact
size. Ordinary benchmark runs reject an unexpected metadata profile. The
existing original clock/timer checks and clean-exit checks remain required.

Build, actual total shared-SRAM cost, regression and matched timing results
must be established before considering adoption. A faster result is not
assumed: the extra address/owner checks may outweigh the cheaper metadata
loads. Keep the existing 4,096-byte total SRAM growth bound against Build 34.

Build 49 succeeded (460.95 seconds on this run), with 5,007,115 application
bytes in PlatformIO accounting and 154,836 static data/BSS bytes. Actual total
shared SRAM growth is 3,800 bytes against Build 34, or 464 bytes above Build 48:
the 200-byte metadata slot plus code reservation/alignment costs. All selected
17 CPU routines and the four memory entry points remain in IRAM. The existing
4,096-byte bound passes. Application image SHA-256:
`5df26f21a76c0acc3c8128e7d58d8c1d482d48723a9bd034db1def1b4d09bcbf`.

Flash 37 verified all segments. Full injected-control regression 49 passed,
including real e7e5/d2d4 ROM replies, both 40,960-byte read comparisons, saved
low-page read/write restoration, tap invalidation, slot release/reclaim on ROM
switch, launcher/Virginia/Bots/gestures and clean task exits. This is automated
USB control evidence; it is not new physical finger/phone acceptance.

Both full, uninstrumented 180-wall-second search measurements completed with
34 retained windows, original clocks/timers and clean ROM exits:

| Model / case | Adopted Build 48 | Metadata candidate 49 | Relative change |
|---|---:|---:|---:|
| MM IV / Spanish centre | 30.789% | 27.847% | -9.558% |
| MM V / Dragon attack | 32.684% | 29.572% | -9.522% |

Retained intervals cover 170.937 / 170.623 seconds. Minimum internal free heap
was 22,916 bytes in both searches; minimum free worker stack was 7,292 / 7,600
bytes. The metadata startup contract and low-page tap test passed. Neither
ROM completed a bestmove during the measured search. These are the same
established histories and `le 9` setting; internal ROM board/depth readback
remains unavailable.

The experiment is rejected as the default. Cheaper metadata loads did not
offset the additional path selection and altered compiled code in these runs;
the tests do not isolate the exact cost of each added operation. This result
does not establish that all internal-memory placement strategies are slower.

Default-off Build 50 succeeded in 394.30 seconds, with the original 5,005,711
application / 154,636 static data/BSS byte accounting and 3,336-byte total SRAM
growth. Its full image differs from Build 48 in only 65 bytes: the 32-byte
embedded ELF hash at 0xb0–0xcf, image checksum and final SHA-256. All other image
bytes match exactly. Build-50 image SHA-256:
`a8e44bc678534340389866bed2a0805b6f2cee3fec9a2a4caca551142e5567f2`.
The linker verifier confirms the original IRAM placements and absence of the
metadata marker. The rebuilt image is a configuration check, not a flashed
firmware claim.

Flash 38 instead restored the exact qualified Build-48 checkpoint, after
checking its saved image hashes and retaining all existing flash guards.
Full injected-control regression 50 passed on that restored firmware. The
device remains on Build 48; the failed experiment remains an explicit opt-in
for reproducibility. Both the linker verifier (`--hot-metadata`) and benchmark
(`--expect-hot-metadata`) now reject a mismatched metadata profile.

A fresh MM-IV control on the restored exact Build-48 firmware achieved
30.79085% original speed, essentially the previous 30.78933% reference. The
metadata candidate is 9.562% slower against this fresh control. The requested
search lasted 180 wall seconds; 27 intact rate intervals were retained,
covering 135.917 seconds. Interleaved USB output reduces retained coverage;
partial lines were not reconstructed. Original clocks/timers, absence of the
metadata marker and clean exit all passed. No fresh MM-V control was run;
its comparison above uses the earlier qualified reference.

Eight portable build-configuration tests pass, including opt-in defaults and
rejection of combinations with other diagnostic/inline experiments. Final
active device firmware remains the exact qualified Build-48 image; both ROM
regression and this fresh MM-IV speed control passed after restoration.
