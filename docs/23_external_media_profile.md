# Size experiment: unused external-media and catalog paths

## Baseline and scope

Combined Build 13 is the tested baseline: application 5,135,979 bytes, static
internal RAM 159,764 bytes. Source commit:
`1f20829a9d371d6b17f0491c32f511a27fefcab1`.

The current MM IV/MM V machines boot fixed ROM regions and do not require CHD
disk images, external software-list XML catalogs or optional HSI extrainfo.
The generic managers still retain those paths and dependencies in the binary.
The selected profile removes their reachability rather than altering shared
library implementations or blindly deleting archive objects.

## Changes and retained behavior

`tools/prepare_no_external_media.py` creates three separate source overlays:

- `romload.cpp`: reject setting/opening CHD handles, disk-diff opening and
  disk-region processing explicitly. Ordinary ROM-region processing, file
  loading, length/CRC/SHA checks, missing-ROM reports, masks, copy/fill/erase,
  class layout and existing ownership/cleanup are unchanged.
- `softlist_dev.cpp`: reject parsing an external software-list catalog. This
  is composed after the prior desktop-text/fuzzy-suggestion profile when that
  profile is enabled; the settings can also be selected independently.
- `hashfile.cpp`: both optional HSI extrainfo queries return `false`, leaving
  their result string unchanged, like an absent optional metadata file.
  Despite the filename, this is not the ROM checksum verifier. That verifier
  and the hash implementation remain unchanged.

The generator reuses the established signature/brace-checking replacement
helper. Copyright headers and all unselected function bodies remain intact.
ZIP/7z file support, NVRAM, Lua/UCI, CPU timing, board input, renderer and
application/web assets are not edited. XML used for other purposes is retained.

This does not promise that every CHD/catalog helper disappears: empty-container
cleanup, class interfaces or another retained subsystem can keep shared code
reachable. Measure the linked map and application, not the source/archive size.
Future machines needing disks, catalogs or HSI hints must disable the profile.

## Build selection

- Combined build: `custom_mame_external_media` defaults to `no`; `yes` selects
  the prior ROM loader and metadata query plus whichever desktop-text variant
  is selected for the software-list device.
- Native CMake: `MAME_S3_NO_EXTERNAL_MEDIA` defaults ON; OFF restores those
  sources without modifying independent profile settings.
- Both paths call the same generator. Exactly three external-media source
  selections are required in the combined manifest, including when its native
  database was generated with different profile settings.
- Generated overlays are ignored build products. The upstream tree remains
  untouched, and no PlatformIO configuration edit was needed for this pass.

## Validation

Build 14 passed in 373.24 seconds, compiling only the three selected media
sources and relinking. Application size is 4,975,739 bytes versus 5,135,979 in
Build 13: 160,240 bytes saved (156.5 KiB / 3.12%). Static internal RAM falls
from 159,764 to 158,692 bytes, saving 1,072 bytes.

The mapped pugixml contribution is absent. CHD and chdcodec contributions
remain at 3,807 and 1,848 bytes respectively; do not describe the entire CHD
library as removed. Software-list helper code also remains. The final image
contains the explicit disabled-CHD/catalog errors and selects the new loader.
Six raw-ROM source sections, including checksum verification, are identical
to upstream; their local audit is `rom-loader-source-audit-build14.json`.
Application assets are byte-identical to the accepted Build 13 image.
Flash 10 verified all written segments.

Device regression `bauerino-device-12-no-external-media --no-captures` passed
actual MM IV e2e4/e7e5, MM V d2d4, both result-0 exits with heap integrity,
and return to Virginia. It also passed launcher/gestures, Virginia preview,
Bot selection/all 215 portraits, UCI Options and board/notation. MM V reported
15.87% average speed, consistent with the prior approximately 16%; no speed
gain is claimed. No new screenshots or physical finger/web acceptance were
requested for this core-only change.

Exact images, source delta, map and device evidence are saved locally under
`backups/takeover-20261110/combined-build14-no-external-media`. Build 13 remains
the prior source/firmware rollback. The common-SDK combined target is the
device target; native CMake wiring is not a separately validated firmware
result. The preserved user-confirmed cold-start/touch/web baseline is Build 09.
