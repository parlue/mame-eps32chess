# Unconnected 6502 SYNC output

Reference: Build 25 CPU unity, MM IV 22.90-22.93% and MM V 24.55-24.57%
original speed in two complete short ROM/device regressions.

Upstream MAME 0.220 `m6502_device::init()` uses `sync_w.resolve_safe()`.
The `devcb_write` implementation inserts a dummy function when no output is
bound. Every prefetch then invokes it for both ASSERT and CLEAR, including
default-space lookup and the callback machinery. The current Mephisto MM IV
and MM V driver binds no SYNC consumer.

The generated CPU overlay uses `resolve()` and guards all five call sites:
one reset CLEAR, two prefetch ASSERTs and two prefetch CLEARs. A resolved,
connected output still receives the same calls in the same order. The CPU's
`sync` state, PC/NPC, opcode fetch, interrupt decisions and cycle accounting
remain unchanged. Upstream source is not modified in place.

A startup probe uses actual `devcb_write_line` objects to verify that an
unbound resolved output is empty and a lambda-bound output receives ASSERT
then CLEAR. Temporary callbacks are destroyed before CPU execution. This
probe does not constitute a test of a complete machine with a SYNC-bound
device; the two supported ROMs both use the unbound path.

## Results

Final Build 27 succeeded in 157.23 seconds: 5,002,899 application bytes and
154,612 static internal RAM bytes. It contains 297 MAME objects, exactly one
unity CPU object, no stale separate CPU objects and the same producer flags
as Build 25. Assets, partitions, ROM filesystem and the 28 KiB worker stack
were verified unchanged before guarded Flash 19.

| ROM | Build 25 unity reference | Build 27 final | Build 27 repeat |
|---|---:|---:|---:|
| MM IV | 22.90-22.93% | 26.45% | 26.53% |
| MM V | 24.55-24.57% | 29.40% | 29.44% |

SYNC removal adds approximately 16% and 20% speed over the preceding unity
reference. Together with unity, the gain over published Build 24 is about
23-27%. These are MAME's original-speed percentages from short opening/input
tests, not a sustained search benchmark. The 50% practical target remains
unmet; the remaining factor is approximately 1.7-1.9x.

Both final regressions passed actual MM IV e2e4/e7e5 and MM V d2d4, four clean
result-0 exits with intact heaps, both startup memory and SYNC probes per run,
launcher gestures, all 215 Bot decodes, Virginia, dynamic UCI options and
Board/Notation. Minimum reported stack reserve was 7,372 bytes. This is not
a new physical finger/phone acceptance or an unbounded reload stress test.

Intermediate Build 26 also passed a full device regression (26.50% / 29.66%).
Build 27 changes only its diagnostic newline; the final measurements above
are used rather than selecting the higher preliminary MM V result.

The CPU source audit reverses precisely the five guarded calls and init probe
to reproduce Build 25 byte-for-byte. CMOS instruction and page-adapter inputs
are unchanged. Compiler producer and archive audits passed. The native project
uses the shared SYNC generator, but has not been independently rebuilt or
device-tested here; unity remains combined-builder only.

Exact private checkpoints `combined-build25-cpu-unity`,
`combined-build26-idle-sync` and `combined-build27-idle-sync` retain matching
firmware, source and raw evidence. Build 24 is retained as the previous
published rollback. ROMs and binary backups are not distributed in Git.
