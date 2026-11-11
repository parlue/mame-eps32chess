# Stability before additional CPUs

Scope agreed with Dirk: stabilize the existing MM IV/MM V MAME port first.
General feature removal and additional CPU cores follow afterwards. Native
LCD integration remains deferred; current core firmware links explicit no-op
LCD hooks and does not include the display image or SPI driver.

## Ownership correction under test

Original 0.220 `address_space_specific` allocates read/write dispatch roots,
but has no derived destructor. Its base destructor only releases unmap/nop
handlers. The dispatch destructors already release their child references.
The S3 build now generates an overlay adding a destructor which unreferences
both roots and any delayed handler references, before the base destructor.
The original source tree is unchanged; configuration rejects an unexpected
upstream destructor or a missing insertion anchor.

The previous device load-cycle test retained approximately 6.7 MB and failed
on the second ROM. The correction is not considered verified until the same
device can repeatedly alternate both ROMs with valid UCI replies.

## Device acceptance checks

- At least six alternating MM IV/MM V loads, without resetting between loads.
- Correct UCI engine identity and ready handshakes on every load.
- Legal opening response from the actual ROM and clean result-0 quit.
- Heap integrity check after every destroyed machine.
- After warming up both drivers, no sustained loss above 64 bytes in either
  internal RAM or PSRAM. The initial broad 16 KiB threshold helped identify
  the major leak, but was tightened after investigating the small drift.
- Longer bounded search with clean quit and no allocation failures or panic.
- Preserve logs, firmware, ELF and hashes for the validated checkpoint.

These checks cover the standalone core. They do not demonstrate a completed
Bauerino application, original emulation speed, or display/BLE/Wi-Fi operation.
Previously measured emulation speed was approximately 13–18 percent.

## Build history

- Build 17: configuration failed because installed CMake 3.16 does not support
  `file(CONFIGURE)`. Replaced with content comparison and conditional WRITE.
- Build 18: full rebuild passed in 764.46 seconds, flash 9,140,200 bytes.
  Existing host UCI/Lua tests passed. Build 19 adds the final post-exit heap
  integrity/snapshot probe; physical validation follows that incremental link.
- Build 19: passed in 97.60 seconds; firmware 9,141,264 bytes. Flash 10
  completed with verified hashes on the guarded Waveshare COM8 device.
- Load cycle 02 running: first MM IV load completed two actual UCI games
  (`e2e4 -> e7e5`, new game, `e2e4 -> e7e6`), clean quit. Idle PSRAM
  8,312,940 bytes, internal RAM 263,307 bytes, integrity OK. MM V then
  successfully started without resetting the ESP (previously failed here).
- Load cycle 02 finished: six loads, twelve real opening replies, all clean
  exits and all heap integrity checks OK. PSRAM was exactly 8,312,940 bytes
  after every cycle. Internal free RAM fell from 263,307 to 262,815 bytes
  (492 bytes); this smaller drift remains under investigation, not dismissed
  as proven stable. Build 19 firmware preserved separately, SHA256
  `0e1020e421e3564a78575db423755900873accfc3aae8b324f595a48f28bb474`.
- Build 20 passed in 98.37 seconds. Adds an idle-only `heap` diagnostic
  command: allocator-free ROM printing of small allocated internal blocks.
  It runs only on explicit command, not on the emulation path.
- Heap/search cycle 01 passed four alternating runs, each with 60 seconds
  search after `go`, followed by clean quit and intact heaps. No allocation
  failures. All idle PSRAM values again exactly 8,312,940 bytes.
- Comparing idle snapshots after the same MM IV driver (snapshots 1 and 3)
  reveals four additional 44-byte blocks. Their complete field layout matches
  `memory_access_cache<0,0,ENDIANNESS_LITTLE>` (space, notifier IDs 0/1,
  16-bit mask, cached ranges and dispatch pointers). Original m6502::init
  allocates two raw caches; `memory_interface` and `mi_default` destructors
  are empty. Four retained caches plus allocator overhead explain the small
  internal drift across two more loads.
- Added a second generated overlay for m6502.cpp: device_start creates a
  local subtype of mi_default with initialized-null cache pointers and a
  destructor deleting both caches. The CPU headers/ABI and upstream checkout
  remain unchanged. Destruction occurs before the device's address spaces.
- Build 21 did not regenerate CMake, so its success was NOT a test of that
  change. Removed only this project's CMakeCache.txt and started Build 22;
  verified the generated m6502_s3.cpp contains the owning interface.
- Build 22 passed in 153.49 seconds, flash 9,141,632 bytes. Only the new CPU
  overlay required compilation after CMake regeneration, followed by relink.
  Final physical regression with both ownership fixes follows Flash 12.
- Flash 12 verified. Final load cycle 03 passed six alternating ROM starts,
  valid ROM opening replies, all clean quits, all integrity checks. Test used
  `--max-idle-loss 0`, so no net loss after warming up both drivers was allowed.
  PSRAM was exactly 8,312,940 bytes after every run. Internal free RAM:
  263403, 263387, 263403, 263387, 263403, 263391 bytes. Driver-specific
  variation is bounded and does not grow. Same-driver heap snapshot diff:
  11 changed blocks/764 payload bytes replaced by exactly 11/764; no additional
  retained caches. Logs: takeover-load-cycle-03.log, takeover-heap-fixed-diff.txt.
- Final search regression started: each driver gets up to 120 wall seconds
  after `go` on h2h4/le9/Speed0, then quit. This tests search abort/lifetime,
  not a promise of a completed engine reply or original speed.
- Final search regression PASSED: MM IV quit after 120.17 seconds, MM V
  after 120.05 seconds of search. Both clean result-0 exits and intact heaps,
  no allocation/UCI errors or panic. PSRAM 8,312,940 bytes for both; internal
  RAM 263,403 / 263,387 bytes, identical to opening-test idle values.
  Search steady-state speed remained approximately 14–15 percent; original
  speed is NOT achieved. `takeover-search-stable-01.log` is the evidence.

## Validated checkpoint: standalone core v1

Build 22 / Flash 12 is the first tested checkpoint with both lifetime fixes.
Validated on the actual Waveshare: six alternating load/opening/quit cycles
with no post-warm-up memory loss, plus two 120-second search/quit cycles.
The earlier build also completed twelve opening replies with repeated New
Game and four 60-second search/quit cycles, before the small cache leak fix.
These are bounded regression tests, not full-game endurance certification.

Saved under `backups/takeover-20261110/core-stable-v1/`: firmware, ELF,
bootloader, partition table, project-owned sources, evidence and SHA256 manifest.
ROM image identity is recorded; private ROM bytes are not added to the source
archive. The original 16 MiB display-demo flash backup is preserved separately.

Remaining work: performance, full Bauerino application integration including
LCD/BLE/Wi-Fi, and only then application-level endurance testing. General MAME
slimming/additional CPU cores were not mixed into these correctness changes.
The current flashed image is the standalone UCI test core, with the display
implementation deferred. No new physical LCD screenshot was taken.
