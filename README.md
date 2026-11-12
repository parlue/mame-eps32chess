# MAME ESP32 Chess / Bauerino

MAME/MESS 0.220 for ESP32-S3, with a cooperative Lua/UCI bridge and the Bauerino frontend.
Target: **Waveshare ESP32-S3-Touch-AMOLED-2.16**, 480 x 480, 16 MiB flash and 8 MiB Octal PSRAM.

## Verified development snapshot - 12 November 2026

- Combined Build 53 combines direct qualified memory pages, bounded CPU/shared-memory IRAM and reduced sample generation for discarded DAC audio, with the startup animation, launcher, Virginia, Bots, Wi-Fi portal and web frontend. Retro runs in the same application without a reboot.
- Actual MM IV ROM response: e2e4 -> e7e5. Actual MM V ROM opening: d2d4.
- The device regression passed clean result-0 exits, intact heaps, return to Virginia, launcher gestures and all 215 Bot portrait decodes.
- Physical cold start, animation, finger touch and phone web access were confirmed with Build 09. The current default's exact image passed the injected-gesture frontend/ROM regression and both sustained middlegame runs in Build 52. Build 53 verifies the exact default-application hash; that image is already running on the device. This is not a new physical acceptance test.
- Full application: **5,006,971 bytes**; PlatformIO data/BSS: **154,644 bytes**. Total extra shared SRAM versus Build 34 is **3,088 bytes**, within the 4,096-byte profile limit and 248 bytes below Build 48. Qualified RAM/ROM pages use a 2,076-byte PSRAM helper, saving 4,092 bytes; I/O, mapping/bank changes and state restoration retain the MAME fallback.
- Build 27's repeated short opening tests reported **26.45-26.53% original speed for MM IV** and **29.40-29.44% for MM V**, approximately **23-27% faster** than Build 24. CPU-only unity compilation and skipping unconnected SYNC callbacks provide this gain. Startup memory/read/write/tap and empty/connected callback validation pass.
- Build 28 keeps that CPU/clock/profile code unchanged and adds a sustained middlegame reference: **MM IV 19.79%** in a closed Spanish centre and **MM V 20.67%** in a Dragon attack, each measured during a 180-wall-second search phase after replaying 32 plies. UCI/Lua polling takes a median **25.3% / 26.6% of wall time** in these windows. These are different workloads from the opening tests, not an additional optimization or a same-position comparison between ROMs. No bestmove completed at level le 9 within either window. These Build 28 values are the pre-cache reference.

- Build 29 reuses selection tables/LED names and reads display digits once per scan. The same sustained middlegames now reach **MM IV 24.77% / MM V 26.23%**, with UCI/Lua median wall time **6.5% / 6.9%**. Both 180-wall-second runs exit cleanly. The next practical 50% target is still approximately another factor 2 away.
- Build 30's optional startup-only RAM/ROM probe compares access layers with matching checksums on both ROMs. Build 31 ships with that probe disabled, identical generated CPU source and application size to Build 29. The measurements and compiler assembly identify the remaining indirect fetch path as an A/B-test candidate; no additional search-speed gain is claimed.
- Direct CPU access experiments in Builds 32/33 did not improve sustained speed. The compact version reached **MM IV 24.29% / MM V 25.70%**, about 2% below the previous matched runs. It remains opt-in; normal Build 34 has the exact stable Build-31 application hash. The compact candidate's frontend/ROM regression and both sustained searches passed. See the [experiment and limits](docs/33_direct_cpu_memory.md).
- Build 37 places 16 common instruction routines plus prefetch in internal instruction RAM. The same counter-free sustained middlegames reach **MM IV 26.52% / MM V 28.01%**, **7.1% / 6.8% faster** than Build 29. Extra shared SRAM cost is **2,816 bytes**; app growth is 232 bytes. This is the adopted combined profile, default in Build 38; set `BAUERINO_MAME_HOT_ROUTINES=0` for the former CPU. The original clocks remain unchanged. [Counts, placement checks and limits](docs/34_opcode_hot_routines.md).
- Core timing in Build 41 attributes **13.50% / 14.35%** of search wall time to sound updates. Skipping final stereo conversion alone produced no meaningful gain in Build 42 and remains optional. Build 44 instead requests 11,025 Hz for the discarded 192-kHz DAC stream through MAME's own API: **MM IV 30.19% / MM V 32.11% original speed**, **13.82% / 14.62% faster** than the immediate IRAM baseline. CPU clocks, timer periods and digital DAC writes are unchanged; audible/recording machines keep the original rate. Extra SRAM is **8 bytes**. This is adopted in default Build 45; `BAUERINO_MAME_QUIET_DAC=0` restores the old sample rate. The 50% target remains unmet. [Qualification and limits](docs/37_quiet_dac_stream.md).
- Build 46 moves the four shared paged-memory entry methods into IRAM without changing their bodies: **MM IV 30.79% / MM V 32.68% original speed**. Fresh standard controls in reversed model order give **+1.95% / +1.67%**, confirming a small gain at **512 additional SRAM bytes**. This is adopted in default Build 48; `BAUERINO_MAME_HOT_MEMORY=0` restores the four flash methods. Original clocks/timers and quiet DAC are retained. [Candidate, fresh controls and limits](docs/38_shared_memory_iram.md).
- Direct qualified page pointers in Build 52 reach **MM IV 33.51% / MM V 35.83% original speed**, **+8.83% / +9.62%** over the preceding qualified references. Bank mutations and post-load restoration invalidate the fast path; no extra per-access counters or checks are added. This is adopted in default Build 53, whose complete image matches the tested candidate. `BAUERINO_MAME_FLAT_PAGES=0` restores the old bank-base-per-access strategy. The 50% target remains unmet. [Qualification, memory savings and limits](docs/40_flat_page_pointers.md).
- Current core profiling attributes **75.93% / 75.06%** of instrumented search wall time to CPU execution, versus only **1.48% / 1.59%** to sound. CPU-only `-O3` passed both ROM tests but gives only **+0.07% for MM IV against a fresh O2 control**; MM V is +1.19% against its earlier reference without a fresh repeat. O3 remains opt-in (`BAUERINO_MAME_CPU_OPT=O3`), adding 9,596 application / 256 SRAM bytes. O2 remains the default; Build 56 restores the exact Build-53 image and passes the frontend/ROM regression and fresh MM-IV control. [Current profile, compiler experiment and limits](docs/41_current_profile_cpu_optimization.md).

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
- [CPU-only unity compilation](docs/28_cpu_unity.md)
- [Unconnected SYNC output overhead](docs/29_idle_sync.md)
- [Sustained middlegame benchmark and polling target](docs/30_middlegame_benchmark.md)
- [Model-interface selection scan cache](docs/31_selection_scan_cache.md)
- [Startup memory-path measurements](docs/32_memory_access_probe.md)
- [Direct CPU access experiments](docs/33_direct_cpu_memory.md)
- [Opcode frequencies and bounded IRAM placement](docs/34_opcode_hot_routines.md)
- [Bounded inline memory experiment](docs/35_hot_direct_memory.md): +1.07% / +1.20% in single matched runs, at 1,040 extra SRAM bytes. Kept opt-in; Build 40 restored the exact adopted IRAM application and passed the device regression again.
- [Exclusive core timing and final conversion experiment](docs/36_core_profile_discarded_mix.md)
- [Adopted quiet DAC stream profile](docs/37_quiet_dac_stream.md)
- [Adopted shared-memory IRAM placement](docs/38_shared_memory_iram.md)
- [Rejected low-page metadata SRAM experiment](docs/39_hot_page_metadata.md): about 9.5% slower in both matched middlegames; remains opt-in. Build 48 was restored at the end of that experiment.
- [Adopted direct page pointers and bank invalidation](docs/40_flat_page_pointers.md)
- [Current core profile and optional CPU-only O3 comparison](docs/41_current_profile_cpu_optimization.md)
- [Lua/UCI bridge](integration/chessengine/README.md)

Older documents describe historical intermediate states. Use the later build/test records for current acceptance.

## Licenses

See [NOTICE.md](NOTICE.md), original file headers and included license texts. `LICENSE` contains the Bauerino/Virgin application's GPL-3.0; it does not relicense third-party components.
