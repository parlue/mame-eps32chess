# Direct qualified page pointers with bank invalidation

Reference: qualified Build 48, MM IV 30.789% and MM V 32.684% original speed in
the established 32-ply middlegames. The fresh restored MM-IV control after the
previous experiment reached 30.79085%. Original clocks, timers and quiet DAC
remain unchanged. This candidate is initially opt-in:
`BAUERINO_MAME_FLAT_PAGES=1`. It is now adopted as default in Build 53 after
the qualification below; explicit `0` restores the previous per-access bank path.

Instead of re-reading a bank's vector/current-entry/base on each qualified
access, each page holds only the directly qualified byte pointer. RAM contents
are not copied. The original native-byte, 16-bit LE handler whitelist and
256-byte contiguity tests remain required. Unknown handlers, I/O, taps and
mapping changes retain the normal MAME path. Separate opcode maps retain a
separate table.

Four bank mutation functions in a generated owning memory overlay invalidate
all live fast-page helpers before mutation: `set_base`, `set_entry`,
`configure_entry`, `configure_entries`. Validation that precedes the mutation
remains unchanged. Invalidating even an unrelated bank is conservative. The
fast path stays disabled for that helper's lifetime after invalidation; it is
never rebuilt inside a callback. Intrusive helper registration is bounded by
the number of live helpers, allocated in PSRAM, and unlinks before destruction.
This uses the existing single MAME worker and is not concurrent-worker code.

Only the generated source is patched. Reversing the four added hook calls,
their declaration and the post-load registration yields the exact original input. Upstream class headers and
layouts remain unchanged. The host C++ test compiles the actual four transformed
upstream bodies against a minimal bank fixture; it verifies that invalidation
precedes writes/allocation callbacks, successful results match, and validation
exceptions retain original state. Both generator tests and nine configuration
tests pass. Inline/opcode/probe/metadata experiments cannot be combined with
explicit direct pages. Coarse core timing intentionally inherits this profile.

The device startup test retains all real upstream byte comparisons and the
saved RAM-byte write/read/restore plus tap fallback test. A separate fresh
observer helper independently tests `set_base` with the unchanged current base:
the observer must invalidate and fall back correctly. Its destruction tests
unlinking before the execution helper is freshly built. The benchmark requires
three helper startups, the bank self-test, and no subsequent invalidation
during replay or measured search; a normal run rejects the unexpected profile.

Build 51 succeeded in 476.60 seconds: 5,006,411 application bytes and 154,644
static data/BSS bytes. Actual total shared SRAM growth is 3,088 bytes against
Build 34, or 248 bytes less than the adopted Build 48. IRAM reservation is
3,072 bytes above Build 34. All 17 CPU routines and all four paged-memory
methods retain IRAM placement. A page descriptor drops from 12 to four bytes
on the ESP, saving 4,096 bytes across the two program-space tables before the
small helper registration field. No per-access counters are introduced.

The four generated CPU sources (`m6502_speed_s3.cpp`, both hot instruction
headers and `m65c02_hot_s3.cpp`) remain byte-identical to the adopted Build-48
sources. The change is confined to the page helper and owning memory overlay;
ROM CPU clocks, instruction bodies and cycle accounting are not rewritten.

Build 51 is an initial performance candidate, not yet a new default. It lacks
post-load invalidation for direct restoration of saved bank selection. The
current UCI bridge does not use emulator save/load-state calls.

Build 52 adds a machine-lifetime post-load callback in memory initialization.
The callback holds no helper pointers and invalidates the current helper list,
so temporary self-test helpers cannot leave dangling callbacks. It adds no
per-access work. Generator reversal includes this registration. A complete
save/load-state roundtrip is not part of the current UCI device tests.

Build 52 succeeded in 436.54 seconds: 5,006,971 application bytes, unchanged
154,644 static data/BSS and 3,088 total extra shared SRAM bytes versus Build 34.
IRAM reservation and all 21 selected CPU/memory placements still pass. This is
the complete candidate for subsequent qualification.

Flash 39 and full injected-control regression 51 passed, including both real
ROM replies, original frontend controls, byte comparisons, tap fallback, bank
notification, helper cleanup and clean ROM exits. Initial candidate 51 reached
33.37068% in the MM-IV Spanish centre: +8.379% against the fresh restored
30.79085% reference. Its search retained 30 intact intervals (150.821 seconds).

Flash 40 verified the complete post-load candidate 52. Full regression 52
passed again, with e7e5/d2d4, both memory/bank tests, post-load registration,
launcher/Virginia/Bots/gestures, Board/Notation and clean exits. Both subsequent
180-wall-second searches after the same 32-ply histories passed:

| Model / case | Previous qualified reference | Complete candidate 52 | Relative gain |
|---|---:|---:|---:|
| MM IV / Spanish centre | 30.79085% | 33.50874% | +8.827% |
| MM V / Dragon attack | 32.68375% | 35.82792% | +9.620% |

The MM-IV reference is the fresh exact-Build-48 restored control immediately
preceding this experiment; MM V uses the earlier qualified reference. There
was no new MM-V baseline repeat. These are limited matched measurements, not
a statistical guarantee or a prediction for other positions/ROMs.

Each final search retained 29 intact rate intervals, covering 145.756 / 145.649
seconds. USB output interleaving reduced retained coverage; partial lines were
not reconstructed. Neither ROM completed a bestmove within the search window.
Both retained original 4,915,200-Hz CPU clocks, the original timer period and
quiet DAC contract. Direct-page startup/bank/post-load checks passed, with no
subsequent invalidation during replay/search. No instrumentation was added to
the live access path. CPU wall medians were 76.6 / 75.4%, including preemption.
Minimum internal free heap was 23,628 / 23,624 bytes, worker stack 7,372 / 7,408,
and PSRAM 2,663,740 / 2,663,660. Actual helper size is 2,076 bytes versus the old
6,168: a 4,092-byte saving including its new registration field.

The profile is adopted. Default Build 53 succeeded in 160.96 seconds with the
explicit flat-page environment flag removed. Its complete application image
is byte-identical to qualified Build 52, already on the device:
`e2e0b9cb03fd82b8d8a1b7ea7e18024af3c84d1f90bbc49e1cbc3a4d53a86cd5`.
Application accounting remains 5,006,971 bytes and static data/BSS 154,644.
Total extra shared SRAM remains 3,088 bytes against Build 34, within the same
4,096-byte bound and 248 bytes below Build 48. No duplicate flash is required
for this identical default image. Physical finger/phone acceptance is still
the earlier user confirmation; these new checks use injected USB controls.

`BAUERINO_MAME_FLAT_PAGES=0` restores the preceding memory strategy. Explicit
older metadata/probe/opcode/inline profiles and disabled shared IRAM keep their
original memory strategy by default; explicit direct pages require CPU unity.
Coarse core profiling inherits the new qualified default. Nine configuration
tests verify these interactions. The 50%-of-original target remains unmet.
