# Size experiment: desktop text keyboard and fuzzy software suggestions

## Baseline and dependency finding

Combined Build 12 is the tested baseline: application 5,423,863 bytes, static
internal RAM 159,764 bytes. Source commit:
`8ea8d6147f7fadb60c7501b5ddcf38c5f76c28d8`.

MAME constructs a natural-keyboard helper even though the current ROMs are
operated through direct input-port fields. This helper supports typed/pasted
text, character alternatives and timed character posting. It is separate from
the input-port matrix used by the Lua/UCI bridge.

The large utf8proc contribution is not solely a keyboard dependency. The
retained software-list approximate-name search performs Unicode normalization
and case folding. Removing keyboard support alone therefore does not establish
that the approximately 283 KB Unicode block can be discarded. The selected
profile also omits that diagnostic suggestion search; exact lookup stays
untouched. The unused driver's approximate-search code is not modified.

## Changes and limits

`tools/prepare_no_desktop_text.py` generates two separate overlays from the
unchanged upstream files. It verifies expected function signatures and matches
body braces while masking literals/comments. Copyright headers, class layouts
and constructor member initialization remain unchanged.

- The natural-keyboard object remains valid but idle, with no character map,
  posting buffer or timer. An enabled natural-keyboard option, configuration,
  activation or any public text-posting entry point throws an explicit error.
  Disabled-state queries remain available; dump reports disabled support.
- `software_list_device::find_approx_matches` clears the supplied suggestion
  slots and logs that approximate suggestions are unavailable. Ordinary exact
  lookup and existing missing-software error handling are not replaced.
- Original private keyboard functions may remain in the translation unit;
  removing their registration/reachability lets the linker discard them.
- Unicode codecs, normalization implementation and character tables are not
  rewritten or truncated. Any still-used Unicode functionality remains linked.
  Application rendering, web strings, graphics and language handling are not
  changed. This does not implement the previously deferred language switch.
- CPU execution, memory mapping, ROM bytes, Lua/UCI translation, input-field
  setters and firmware filesystem assets are unchanged.

This profile intentionally does not support injecting typed text into an
emulated home computer, or fuzzy software-name suggestions. Future targets
requiring those features must disable the profile.

## Build selection

- Combined build: `custom_mame_desktop_text` defaults to `no`; set `yes` to
  select both original source files. The manifest must select exactly two
  sources and accepts database entries generated with either native setting.
- Native CMake: `MAME_S3_NO_DESKTOP_TEXT` defaults ON; OFF uses the originals.
- Both entry points call the same generator. Overlays are ignored build
  products; no PlatformIO configuration change was needed for this pass.

## Validation

Build 13 passed in 332.62 seconds, compiling the two selected overlays and
relinking. Application size is 5,135,979 bytes versus 5,423,863 in Build 12:
287,884 bytes saved (281.1 KiB / 5.31%). Static internal RAM remains 159,764
bytes. The utf8proc archive contribution falls from 283,153 to 208 mapped
bytes, while the unchanged Unicode codec object still contributes 1,231 bytes.
Thus the large tables are gone, not every last helper byte. Both generated
overlay objects and the explicit disabled-keyboard error are in the image.

Flash 09 verified all written segments. Device regression
`bauerino-device-11-no-desktop-text --no-captures` passed real MM IV e2e4/e7e5
and MM V d2d4, both result-0 exits with intact heaps and return to Virginia.
The full launcher/gesture, Virginia preview, Bot selection/all 215 portraits,
UCI Options and board/notation regression also passed. MM V reported 15.90%
average speed, consistent with the prior approximately 16%; no speed gain is
claimed. No new screenshots or physical finger/web acceptance were requested.

Exact images, source delta, map and device evidence are saved locally under
`backups/takeover-20261110/combined-build13-no-desktop-text`. Build 12 remains
the prior source/firmware rollback. The common-SDK combined target is the
device test target; native CMake wiring is not a separately tested firmware
build. The preserved user-confirmed cold-start/touch/web baseline is Build 09.
