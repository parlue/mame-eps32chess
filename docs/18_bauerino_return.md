# Full Bauerino frontend return

Dirk requests the original start animation and all existing application
functions back alongside the tested MAME engine. The standalone Retro UI is
an intermediate hardware/core test, not the intended final application.

## Preserved frontend

Original project: `C:/Users/DirkSommerfeld/Documents/projects/espmame`.
Workspace build copy: `output/bauerino-prep`.
The original complete 16 MiB device backup and the confirmed cold-boot/touch
Retro Build 29 checkpoint remain preserved separately.

Bring back the six-stage queen splash, wallpaper, two-page app launcher,
Virginia, Bot selection/portraits, board/notation, game/color/strength controls,
Chessnut/ChessLink, Wi-Fi captive portal and web controls. Analyse and Setup
were placeholders in the original frontend; do not report them as completed
analysis/settings implementations. The requested DE/EN selector is also open.
Retro must use actual ROM/UCI state instead of the old preview game.

## Measured integration constraints

Build 30 reduces the firmware size reported by PlatformIO from 9,776,816 to
4,686,292 bytes (5,090,524 fewer bytes, 52.1%). ROM correctness and lifetime
regression are being checked before declaring this a stable checkpoint.
This is flash reduction; live memory use has not been reduced by this change.

The preserved frontend eagerly allocates 2,548,992 bytes for its framebuffer,
wallpaper, piece tiles, six queen images and five app cards, before Bot caches,
Virginia hash and network/BLE allocations. The running native MAME test has
only about 100 KiB of free PSRAM. Simply adding the original framebuffer would
therefore fail even though the reduced binary fits flash.

The new allocator diagnostics distinguish a real CPU address space of 16 bits
from MAME's generic **dummy space of 32 bits**. Upstream `machine.cpp` builds
that dummy space with a uniform full-range handler that throws on access.
`handler_entry_dispatch_lowbits(32,0,0)` returns 14; each read/write root
therefore contains 262,144 pointers and 262,144 two-offset ranges: about 3 MiB
per root on this 32-bit target. This explains most of the live PSRAM cost.
Do not narrow the emulated CPU or dummy address ranges as a shortcut.
A consistent smaller dispatch fan-out can preserve the full address semantics
while avoiding those large roots; all translation units must use identical
template definitions. This is a separate change requiring the same ROM tests.

## Integration work in progress

Build 31 generated a complete core source/header view with an added 24-bit
dispatch level, retaining full 32-bit addresses. The first build reused the
allocator object, so its successful ROM test did not show the expected RAM
reduction. Build 32 explicitly rebuilt the component objects. Flash 21 then
passed actual MM IV/MM V reply/quit tests with intact heaps, and approximately
6,395,000 bytes of free PSRAM while running. Idle PSRAM returns to 8,311,432
bytes. Saved checkpoint: `retro-core-compact-build32`.

The combined application is staged under `mame-s3/bauerino`, preserving the
original source project. Its SCons extension rebuilds the selected 298 MAME
sources against the frontend's Arduino 3.3.5 / IDF 5.5 SDK. No old native MAME
archive is linked into that application. Build 01 failed because pre-script
clones preceded compiler setup; Build 02 moves the extension to the post
phase and is in progress.

The existing display framebuffer renders the device-tested Retro state
machine. Its native SPI/touch backend is replaced by the existing Arduino
backend. A mutex protects shared UI state; only the MAME worker calls the
Lua VM. Leaving Retro requests quit and waits for worker cleanup before the
launcher resumes. Virginia game starts are rejected while Retro is running.
This is coded, not yet claimed verified on the device.

Combined Build 02 stopped at the assembler asset path, corrected to the
project-relative path used by the successful first assembly. Build 03 compiled
all MAME sources but linked against Arduino's no-RTTI C++ multilib. Adding
`-frtti` to the link selects the installed RTTI-enabled library. Build 04 then
passed: 6,153,407 bytes of flash and 176,756 bytes of static internal RAM.

Full Flash 01 verified all segments but the boot test failed: display/panel
initialization failed with approximately 9.5 KiB of internal heap left, then
Virginia's fstream setup aborted while creating a newlib lock. The backtrace
was resolved against the exact ELF. Device was held in download mode while
correcting this; it was not declared a working restored frontend.
Build 05 explicitly enables PSRAM for ordinary malloc/new data at setup,
after Serial initialization. SDK/FreeRTOS internal-capability stacks and DMA
allocations retain their internal allocation requirements. Build passed in
115.14 s, 6,153,403 bytes of flash; Flash 02/device regression pending.

Build 05 booted with the restored animation, menus, Virginia preview and Bot
selection. All 215 portraits decoded. Internal free heap remained about 5.7
KiB, so a new 32 KiB MAME task could not be allocated. Build 06 moves the exact
Virgin history/continuation/countermove/LMR arrays (41,984 bytes) and the 7,168
byte Retro command queue into PSRAM. The existing rule/UCI/layout/Bot tests
pass, including actual Virgin searches, stops and session restarts, while
executing the same search-workspace branch with host allocation stubs.
Static internal RAM falls from 176,756 to 127,620 bytes. With about 55 KiB free
after boot, fragmentation still prevented the large dynamic task stack.

Build 08 therefore gives the permanent MAME worker a static aligned 32 KiB
internal stack and static TCB, initialized before Virgin/Wi-Fi/BLE. Total
static RAM including that reserved stack is 160,740 bytes. There is no reboot
between applications. Device test `bauerino-device-06 --no-captures` PASSED:
launcher/gestures, Virginia board/notation preview, Bot selection and all 215
portraits, actual MM IV e2e4/e7e5, actual MM V d2d4, both result-0 exits with
intact heaps, and return to the launcher/Virginia. Running internal free heap
was about 20 KiB and PSRAM about 2.6 MiB. Idle after the ROMs was about 4.82 MiB
PSRAM. Steady-state emulation was about 14-16% of original speed.

USB screenshot transfers were intermittently incomplete at HWCDC's default
100 ms transmit timeout; this is separate from the passed functional test.
Build 09 raises that timeout only during capture, and makes the old automatic
HTTP self-test opt-in via `http-test`. It preserves AP, captive portal, web
routes and BLE startup. Exact Build 09 firmware/ELF/assets/sources are saved
under `combined-build09`, labelled candidate until its device test finishes.
An attempted Build-08 checkpoint correctly refused because Build 09 had
already replaced the output binary; no Build-09 binary was mislabelled as 08.

Combined partition plan: 8 MiB app, original 0x360000-byte LittleFS assets,
4 MiB ROM FAT/WL image. The guarded flash helper verifies the original full
backup, board identity, image hashes and all partition ranges before writing.
The ROM image remains byte-identical, mounted at its new partition offset.
USB screenshot diagnostics defer capture until the frame has been painted
and serialize worker/stdio output around the binary stream. The new device
test exercises launcher, Virginia preview, Bot choices, swipe, real MM IV
UCI play, board/notation and return to the launcher.

The original frontend uses Arduino 3.3.5 / IDF 5.5; the current core uses IDF
5.3. Rebuild against a common framework before integrating the engine; avoid
assuming binary compatibility between existing precompiled libraries.

## Acceptance for the combined application

Cold start returns to the queen animation and app launcher. Virginia/Bots,
board discovery, portal and web UI retain their previous behavior. Play Retro
opens the tested ROM controls, with dynamic UCI options and actual moves.
Leaving Retro destroys the machine cleanly and returns to the launcher.
Repeated frontend/ROM changes must preserve heap integrity and memory.

## Confirmed combined checkpoint: Build 09 / Flash 05

The complete device regression `bauerino-device-07` passed, including 21
framebuffer captures, all 215 Bot portraits, actual MM IV e2e4/e7e5 and MM V
d2d4, clean result-0 exits with intact heaps, and return to Virginia.
The captures contain the framebuffer sent to the panel, not a camera image
or panel readback. Automated gestures use the physical gesture handler.

The user separately confirmed a USB power cycle, Frost/Fire animation,
launcher and finger-operated Retro Load, and the phone's captive/web page.
The combined application is therefore the accepted baseline for subsequent
MAME work. Its exact firmware, ELF, filesystem, ROM image, source archive and
device evidence are preserved under `backups/takeover-20261110/combined-build09`.

The native Retro path remains the demonstrated ROM-test workflow. Full
physical-board gameplay through that path, additional CPUs, performance work
and the previously deferred Analyse/Setup features are subsequent tasks.
Measured emulation speed is about 14-16 percent, not original speed.
