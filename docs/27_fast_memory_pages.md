# RAM/ROM page access experiment

Baseline: stable Build 21, commit `509b26d103d37999f71ce4654b9253b96b5bc96a`.
Its separate data cache reached MM IV 19.88-19.98% and MM V 21.39% original
speed. The next practical target is at least 50%; this is another experiment
towards reducing the remaining per-access overhead.

## Qualified memory pages

`s3_fast_memory.h` builds 256-byte page tables from upstream `dump_maps`.
Only exact native-byte little-endian fixed-memory and memory-bank handler
types qualify. It requires a full page within one mapping and checks that all
256 handler pointers form a contiguous span. Device handlers, taps, mixed
pages, mirrored discontinuities, unmapped accesses and ROM writes retain the
normal MAME cache/handler route. It does not infer mappings from ROM contents
or hard-code MM IV/MM V addresses.

Fixed-memory handlers in MAME copy their backing pointer during installation;
the page uses that same fixed pointer. Real bank handlers instead retain a
`memory_bank` object and query `base()` on each access. This is necessary:
MAME 0.220 `set_entry`/`set_base` can change a bank without notifying general
address-space map listeners. No snapshot of a bank's current backing pointer
is retained for future accesses. Linear offsets are derived from the actual
handler's pointer calculation, covering its mask/base addressing.

An address-space change invalidates the complete page route for the remaining
machine lifetime, before later accesses can dereference an outdated mapping.
It then uses the established caches and their upstream invalidation. Page
tables are not rebuilt inside remapping callbacks. Separate opcode spaces
have a separate read table and notifier; current machines share the program
space. Notifiers and tables are owned and released with the CPU interface.
CPU headers, instructions, emulated clock, interrupts and driver maps are
unchanged. The CPU-only O2/jump-table profile is retained.

## Startup validation

Before CPU execution, the candidate compares every byte of qualified readable
pages with upstream handler reads. It tests a reversible 0x55/0xaa write/read
on RAM that has the same read/write backing pointer and restores the byte.
It installs a temporary read tap on that RAM address, checks page invalidation
and actual invocation through fallback, then removes it. A fresh page object
is constructed after that test outside any remapping callback. Device pages
are never read by this validation. Tests fail explicitly on a mismatch.

The temporary tap may leave the last RAM page split in MAME's mapping; that
page safely falls back if it is no longer represented by one complete span.
The test is not a comprehensive CPU instruction suite or a bank-switch test;
dynamic bank handling is established by using the current upstream bank base.

## Comparison modes

Existing diagnostics remain available. `mame-speed 1` is the Build-21-style
data lookup cache. Bit 2 (`4`) selects page access, so `mame-speed 5` combines
pages with the data cache and is the candidate default. Bit 1 (`2`) still
selects the older optional internal-CPU placement experiment and is not used
for this comparison. Changes while the worker is busy are rejected.

The first Build 22 attempt was stopped before linking/flashing after source
review distinguished fixed handlers from true banks. Build 23 incorporated
both paths but failed to compile an incorrect `address_space::machine()` API
reference. Build 24 uses `space.device().machine()` and is the device candidate.
These intermediate attempts were not flashed; stable Build 21 was preserved.

## Results

Build 24 succeeded in 325.74 seconds: application 4,996,735 bytes, static RAM
154,604 bytes. Flash 16 verified all segments; assets/partitions/ROM image and
the 28 KiB worker stack remain unchanged. The instruction/timing source outside
CPU start, the new include and mode constant is identical to Build 21.

| ROM | Published Build 21 | Build 24 mode 1 reference | Mode 5 pages | Default reset repeat |
|---|---:|---:|---:|---:|
| MM IV | 19.88-19.98% | 18.59% | 21.48% | 21.40% |
| MM V | 21.39% | 19.86% | 23.47% | 23.27% |

The repeated page results improve over the published baseline by approximately
7-10%. The same-binary reference is slower than published Build 21, so its
larger 15-18% ratio must not be presented as the gain over the prior release.
Compiler call-graph/code-placement changes can affect the reference even when
its source path is semantically unchanged. These are short opening/input tests,
not sustained search measurements or proof of the 50% target.

All three full regressions passed: e2e4/e7e5 for MM IV, d2d4 for MM V, six clean
result-0 exits with intact heaps, return to Virginia, launcher/gestures, all 215
Bot portraits, UCI Options and board/notation. Each of the four paged ROM starts
passed comparisons over 40,960 bytes, RAM write/read/restore and actual tap
invalidation/fallback. The only invalidations were the two expected startup
tests per paged run. Initially 160 read/32 write pages qualified; after the
temporary read tap, 159 read/32 write pages qualified for the split last RAM
page. The helper itself is 6,168 PSRAM bytes, plus notifiers/test bookkeeping;
opcode space is shared for these machines.

Minimum observed worker-stack reserve was 7,292 bytes. Final post-exit PSRAM
for MM IV/MM V was 4,819,004 / 4,818,944 bytes in both page runs, compared with
4,818,988 / 4,818,924 in the reference. No retained table allocation is evident
in these checks. Physical finger input/phone access and new screenshots were
not separately requested. Native CMake uses the shared CPU generator/header
but has not been independently device-tested.

Exact source, images, audit and successful device evidence are saved in
`backups/takeover-20261110/combined-build24-fast-pages`; Build 21 is retained.
