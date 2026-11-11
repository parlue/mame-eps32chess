# MAME: measurements after frontend restoration

## Accepted baseline

Combined Build 09 / Flash 05 is preserved with SHA-256 hashes and exact ROM,
filesystem, firmware/ELF, source archive and device evidence. The user confirmed
cold start, animation, launcher, physical touch and phone web access. Actual
MM IV e2e4/e7e5 and MM V d2d4 and clean exits passed the device regression.
Steady emulation speed was approximately 14-16 percent of the original.

The 65xx memory specialization has already reduced the standalone application
from 9,776,816 to 4,686,292 bytes. Compact dispatch segmentation recovered about
6.28 MB of live RAM in the standalone core. These are different measurements:
flash savings and available RAM do not establish increased CPU speed.

## First diagnostic: Build 10

The existing five-second `[MAME RATE]` report is retained. A separate
`[MAME PROFILE]` wall-time report measures:

- `uci`: UCI open/poll, Lua runtime, command handling and reply delivery.
- `display`: digit lookup and platform display notification/flush.
- `yield`: actual time across the existing one-tick FreeRTOS delay.
- `outside_osd`: all remaining wall time, including CPU emulation, MAME core
  video, scheduler, other tasks and reporting overhead. This is not sampled
  CPU time and must not be presented as the CPU interpreter's share.
- `frames`: OSD calls in the measured interval, with its actual duration.

Mutex waits and preemption are included in the section where they occur.
In the combined application display notifications do not draw the framebuffer:
the frontend task renders it separately, so its effect can also occur in
`outside_osd` or in mutex waits. Initial VM opening is visible in the first
window; compare steady windows during equivalent ROM searches separately.

This diagnostic changes neither CPU instructions nor timing configuration,
Lua scripts, memory maps or the watchdog yield policy. A speed improvement
is not claimed. Build and device results are recorded below when available.

Build 10 succeeded in 122.68 seconds: application 6,154,599 bytes, static
internal RAM 160,788 bytes. Flash 06 verified every written segment.
`bauerino-device-08-profile --no-captures` passed the complete functional
regression, including MM IV e7e5, MM V d2d4, result-0 exits, intact heaps and
return to Virginia. New framebuffer captures were deliberately skipped;
Build 09 contains the accepted 21 screenshots. Build 10's USB test does not
replace the user's earlier physical cold-start/touch/web acceptance.

The recorded steady windows with at least 40 OSD calls and UCI below one
percent have median shares: UCI 0.5%, display 0.2%, yield 1.0%, outside-OSD
98.3%. This selection excludes VM opening and heavier command windows; the
complete unfiltered measurements are retained in `profile-summary.json`.
MM V's overall reported speed was 16.01%, consistent with the baseline.
The measurements justify deeper CPU/scheduler/memory investigation; they do
not establish which part of outside-OSD is responsible. Frontend drawing on
another task is not independently profiled by this diagnostic.

Images, exact source delta from Build 09 and evidence are saved separately
under `backups/takeover-20261110/combined-build10-profile`. Build 09 remains
the physically accepted rollback, unchanged.

## Next decisions

Use the measured shares to select an experiment. Preserve exact Build 09 as
rollback. Compare the same ROM/input sequence and log rate, real moves, clean
quit, heap integrity and repeated-load memory. If time remains mostly outside
OSD, instrument scheduler/CPU execution before modifying dispatch or moving
selected hot data into scarce internal RAM. If Lua or yield dominates, isolate
that cost first. Keep board input, web access and touch responsive.

Additional CPU families follow once we have memory and timing headroom and a
matching native bus profile; the present 8-bit LE restriction is explicit.
