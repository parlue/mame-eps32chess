# Size experiment: select the observed address-root widths

## Baseline and scope

Combined Build 11: application 6,019,439 bytes, static internal RAM 159,764
bytes. Its real MM IV/MM V regression passed. Exact images and evidence remain
in the private local checkpoint; the source baseline is Git commit
`75988cbbd5d60bce8d5e5810346e8276ab9416ec`.

The existing native-8 little-endian allocator still constructs address-space
roots through a switch covering every address width from 1 to 32. This keeps
otherwise unused dispatch specializations reachable in the executable. Actual
device logs show a 16-bit CPU space and MAME's 32-bit dummy space.

`tools/prepare_65xx_roots.py` creates a separate overlay from the already
ownership-fixed/native-8 memory source. It retains exactly the original
16/32-bit constructor cases, including their read and write dispatch classes.
It inserts an explicit rejection of other address widths before allocation
or mask calculation. The CPU's address range and the full dummy range are
unchanged. Logical byte/word/dword/qword accesses, masks, mirrors, mapping,
delegate handlers, cache interfaces and cleanup remain the original code.

This is an explicit restriction on supported device configurations. A later
CPU/device with another address width requires extending the root profile or
disabling it; it must not silently run with a truncated address mask.

## Build selection

- Combined build: `custom_mame_65xx_roots` defaults to `yes` in the post script.
  It accepts `no` to select the prior all-width owning/native-8 source. No
  PlatformIO configuration edit was needed for this experiment.
- Native CMake: `MAME_S3_65XX_ROOTS` defaults ON and requires
  `MAME_S3_65XX_MEMORY`. Disable both to restore generic native bus allocation.
- Both entry points call the same generator. The combined manifest must select
  exactly one memory source and handles either base or root-overlay filenames.
- Unchanged MAME sources stay in the separately pinned upstream tree. All
  generated overlays are local build products, excluded from Git.

The generator verifies the ownership/native-8 prerequisite and the exact
original 32-case switch; unexpected upstream structure causes a build failure.
The new runtime guard precedes all handler allocation, avoiding constructor
leaks when an unsupported address width is requested.

## Validation

Build 12 passed in 371.51 seconds, recompiling only the selected memory source
and relinking. Application size is 5,423,863 bytes versus 6,019,439 in Build 11:
595,576 bytes saved (581.6 KiB / 9.89%). Static internal RAM remains 159,764
bytes. The linked image retains the unsupported-width error. Its map contains
the new root overlay, with no retained prior all-root memory object.

Flash 08 verified all written segments. The device regression
`bauerino-device-10-address-roots --no-captures` passed actual MM IV e2e4/e7e5,
MM V d2d4, both result-0 exits with heap integrity and return to Virginia.
Both 16- and 32-bit spaces were observed for each machine. The full regression
also exercised launcher/swipes, Virginia preview, Bot selection/all 215
portraits, UCI options and board/notation. No new screenshots were requested.

The MAME memory objects' mapped flash sections fell from 1,339,978 to 747,379
bytes. Section attribution differs from the complete application saving;
5,423,863 bytes is the application metric reported by the successful build.
Static RAM and live ROM-space requirements are not reduced by this change.
MM V's reported average speed is 16.02%, consistent with the prior result;
no speed improvement is claimed.

Exact images, source delta, map and device evidence are preserved locally in
`backups/takeover-20261110/combined-build12-address-roots`. The common-SDK
combined build is the device target for this pass; native CMake configuration
is not a separately validated firmware result. The original physical
cold-start/touch/web acceptance remains the preserved Build 09 result.
