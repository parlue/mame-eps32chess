# MAME ESP32 Chess / Bauerino

MAME/MESS 0.220 for ESP32-S3, with a cooperative Lua/UCI bridge and the Bauerino frontend.
Target: **Waveshare ESP32-S3-Touch-AMOLED-2.16**, 480 x 480, 16 MiB flash and 8 MiB Octal PSRAM.

## Verified development snapshot - 11 November 2026

- Combined Build 24 includes the startup animation, launcher, Virginia, Bots, Wi-Fi portal and web frontend. Retro runs in the same application without a reboot.
- Actual MM IV ROM response: e2e4 -> e7e5. Actual MM V ROM opening: d2d4.
- The device regression passed clean result-0 exits, intact heaps, return to Virginia, launcher gestures and all 215 Bot portrait decodes.
- Physical cold start, animation, finger touch and phone web access were confirmed with Build 09. Build 24 passed three injected-gesture regressions; this is not a new physical acceptance test.
- Full application: **4,996,735 bytes**; static internal RAM: **154,604 bytes**. Qualified RAM/ROM pages use a 6,168-byte PSRAM helper; I/O and mapping changes retain the MAME fallback.
- Repeated short ROM tests report **21.40-21.48% original speed for MM IV** and **23.27-23.47% for MM V**, approximately **7-10% faster** than published Build 21. Startup memory/read/write/tap validation also passes. The next practical target remains **50% original speed**, approximately another 2.1-2.4x. These opening tests are not a complete search benchmark.

Retro currently provides a limited ROM-test workflow: Load, dynamic UCI Options, Board, Notation, Move and Quit. Full physical-board gameplay through native Retro, additional CPU families and deferred Analyse/Setup features remain future work.

## Source layout

| Directory | Contents |
|---|---|
| `bauerino/` | Combined Arduino/IDF application, touch, web, Virginia/Bots and MAME worker |
| `esp32s3/` | Native ESP-IDF port, MAME component, OSD and shared Retro state machine |
| `integration/chessengine/` | Cooperative Lua/UCI runtime and MM IV/MM V interfaces |
| `tools/` | Source overlays, size reports, flashing and device regression tools |
| `docs/` | Development history and measurements; raw logs/backups remain local |

## Build prerequisites and current limits

This is the tested development source snapshot. The combined build still uses the local PlatformIO package/library cache and the native project's generated `compile_commands.json`. A fully independent clean-clone build has not yet been demonstrated. These paths are explicit in `bauerino/platformio.ini` and `bauerino/mame_build.py`.

1. Obtain the unchanged MAME source separately:

   ```powershell
   git clone --depth 1 --branch mame0220 https://github.com/mamedev/mame.git ref/mame0220-src
   ```

   [UPSTREAM.md](UPSTREAM.md) pins the exact reference commit.
2. Configure the native PlatformIO project under `esp32s3/`. CMake generates CPU/layout sources, ownership and memory overlays, and the profile without desktop debugger commands. The combined application recompiles selected MAME sources against its own Arduino 3.3.5 / IDF 5.5 SDK.
3. Build `bauerino480` under `bauerino/`, then its own LittleFS using `buildfs`. See [combined build notes](bauerino/README.md).
4. Supply and verify ROMs locally using the import tools. The guarded flash helper checks partitions, images and the known development board identity; it is not a general installer for arbitrary boards.

**No ROMs, firmware binaries, ROM filesystem images or full-flash backups are distributed here.** Files under `bauerino/data` are application artwork and Bot assets.

## Current findings

- [Combined frontend and acceptance](docs/18_bauerino_return.md)
- [Performance measurements](docs/19_mame_performance.md)
- [Debugger command size reduction](docs/20_mame_debug_commands_profile.md)
- [16/32-bit address-root size reduction](docs/21_address_roots.md)
- [Desktop text and fuzzy-search size reduction](docs/22_desktop_text_profile.md)
- [External-media and catalog size reduction](docs/23_external_media_profile.md)
- [Movie-recording size reduction](docs/24_movie_recording_profile.md)
- [6502 profiling and dispatch speed improvement](docs/25_6502_speed.md)
- [Independent data lookup cache](docs/26_separate_data_cache.md)
- [Qualified RAM/ROM page access](docs/27_fast_memory_pages.md)
- [Lua/UCI bridge](integration/chessengine/README.md)

Older documents describe historical intermediate states. Use the later build/test records for current acceptance.

## Licenses

See [NOTICE.md](NOTICE.md), original file headers and included license texts. `LICENSE` contains the Bauerino/Virgin application's GPL-3.0; it does not relicense third-party components.
