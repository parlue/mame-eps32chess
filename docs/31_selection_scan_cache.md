# Model-interface selection scan cache

Reference: Build 28 sustained middlegames, MM IV 19.79%, MM V 20.67% original
speed; UCI/Lua median wall time 25.3% / 26.6%.

The importer now adds an optional `prepare_selection()` hook before the shared
bridge's 64-square scan. Only the MM IV model adapter implements it; MM V
inherits that adapter as before. It snapshots the four masked display outputs
once per scan. Digit-code tables and the 64 LED output names are allocated
once per interface instance. Each scan refreshes the snapshot, and LED queries
retain the original short-circuit behaviour. The scan never yields/advances
the machine, so all original repeated digit reads refer to the same emulated
instant. No polling interval, input wait, ROM clock, CPU, scheduler or UCI move
translation is changed. The existing device-signal approach is preserved.

`selection_scan.lua` compares the generated adapter with the original installed
interface across 512 changing signal snapshots: valid/invalid coordinates,
bit 7 masks, single/multiple LEDs and transitions. All 32,768 square results
match; digit reads fall from 256 to 4 per scan and LED read counts are identical.
The ordinary cooperative-runtime host suite passes, including special moves.
Its linker now names only Lua objects rather than including a stale UI-test
`main` via `*.obj`. The initial link failure is retained in local evidence.

Both 32-ply middle-game histories also pass on actual MM IV and MM V ROMs in
official PC MAME 0.220. Replies match the reference: Spanish d4e5 / e3d2,
Dragon d2e2 / d2e2. This is protocol verification, not ESP speed.

## ESP validation

Build 29 succeeded in 309.02 seconds: 5,004,143 application bytes (+264) and
154,628 static internal RAM bytes (unchanged). Guarded Flash 21 verified all
segments. CPU/page adapter, clock/profile source and compiler builder are
byte-identical to Build 28. Assets and the 28 KiB stack were checked unchanged.
The full frontend regression passed both opening ROM replies, memory/SYNC
probes, Virginia/Bots/all 215 portraits, gestures, Board/Notation and clean exits.

Both identical 32-ply middlegames then ran for 180 wall seconds at le 9,
excluding replay and leading intervals, and quit with result 0 and intact heaps.

| ROM | Build 28 original speed | Build 29 original speed | Relative gain | Median UCI wall time |
|---|---:|---:|---:|---|
| MM4 | 19.79% | 24.77% | +25.2% | 25.3% -> 6.5% |
| MM5 | 20.67% | 26.23% | +26.9% | 26.6% -> 6.9% |

Each result includes 34 complete rate windows (171.080 / 171.025 wall seconds)
from search phases lasting 180.953 / 180.125 wall seconds. Minimum stack reserve
was 7,516 / 7,376 bytes and internal free heap 26,548 bytes in both. Free PSRAM
stayed within 2,659,668-2,679,484 / 2,660,156-2,681,336 bytes in these windows.

These are per-position before/after comparisons, one complete sustained run
per ROM/build, not a repeated search suite or same-position ROM ranking.
No bestmove completed in the high-level measurement windows; legal completed
middlegame replies were verified on PC. CPU wall profiles include preemption.
The original emulated clock and device-signal interpretation are preserved.
The practical 50% target remains unmet; approximately another factor 2 is needed.
No new physical finger/phone acceptance is claimed.

Exact private checkpoint: `combined-build29-selection-cache`, including the
matching generated Lua payload and raw evidence. Build 28 remains as rollback.
ROMs and binaries are not distributed. The initial host-only wildcard link
failure is retained separately from the successful runtime and ROM tests.
