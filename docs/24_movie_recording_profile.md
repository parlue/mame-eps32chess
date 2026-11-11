# Size experiment: AVI/MNG recording

## Baseline and scope

Combined Build 14 is the baseline: application 4,975,739 bytes, static internal
RAM 158,692 bytes; source commit `353dbc578c32e0bf78062422284fdc206dcb428e`.
The physical LCD and its screenshot tool use the Bauerino framebuffer rather
than MAME movie capture.

`tools/prepare_no_movie_recording.py` replaces eleven function bodies in
`video.cpp`. Starting or toggling AVI/MNG capture fails with an explicit
disabled-feature error. Recording status returns false; recording frame/audio
and stop hooks have no work. No recording container can be populated through
these entry points. Their class layout and original destruction remain intact.

The constructor, snapshot target, snapshot APIs, screenless timer, frame update,
speed/throttle computations, postload and exit bodies are unchanged. The OSD's
required render target is also unchanged. PNG snapshots and image decoding
are retained; this profile does not remove the renderer or all shared codecs.
ROM loading, Lua/UCI, emulated CPU timing and application assets are untouched.

## Build selection

Combined `custom_mame_movie_recording` defaults to `no`; `yes` restores the
upstream source. Exactly one video source is required in the compilation
manifest, regardless of the native database's selected profile.
Native CMake `MAME_S3_NO_MOVIE_RECORDING` defaults ON; OFF restores upstream.
Both use the same generator and leave upstream sources unmodified. Generated
overlays are local build products. Native CMake wiring is not a separately
validated device firmware.

## Validation

Build 15 completed successfully in 389.97 seconds. Application size falls
from 4,975,739 to 4,955,979 bytes: 19,760 bytes saved (19.3 KiB / 0.40%).
Static internal RAM stays at 158,692 bytes. The linked AVI object contribution
of 13,119 bytes is absent; video source sections fall from 37,550 to 31,394
bytes. PNG remains at 8,546 bytes, while CHD codec and FLAC contributions are
unchanged. These mapped-section measurements differ from image size.

The source audit confirms that all text outside the eleven selected bodies
is identical to upstream. Application LittleFS bytes are identical to Build 14.
PowerShell reports a native-stderr error record for the size tool's informational
"Total image size" message, making the shell exit status 1 despite PlatformIO's
SUCCESS result. The completed image, size report, source audit and verified
checkpoint were inspected before flashing; no compiler/linker failure occurred.

Flash 11 verified all written segments. The first USB test attached after part
of the startup log had already passed and failed its AP-log assertion before
running ROM tests. The subsequent reset captured the full startup and passed
`bauerino-device-13-no-movie-recording-retry --no-captures`: actual MM IV
e2e4/e7e5, MM V d2d4, two result-0 exits with heap integrity, return to Virginia,
launcher/gestures, Virginia preview, Bots/all 215 portraits, UCI Options and
board/notation. MM V average speed was 16.12%, consistent with the baseline;
no speed gain is claimed. No new screenshots or physical touch/web acceptance
were requested for this core-only change.

The exact local checkpoint is
`backups/takeover-20261110/combined-build15-no-movie-recording`. The first
incomplete startup log is retained alongside the successful retry. Build 14
remains available as rollback; the physical acceptance baseline is Build 09.
