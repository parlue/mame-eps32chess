# Combined Bauerino / MAME application

Waveshare ESP32-S3-Touch-AMOLED-2.16, 480 x 480, 16 MiB flash, 8 MiB PSRAM.
This project restores the original Bauerino frontend and hosts the tested
MAME 0.220 MM IV/MM V core in its own task. Device acceptance is recorded in
`../docs/18_bauerino_return.md`; do not infer acceptance from a successful link.

The original frontend source project remains preserved separately. Existing
Virginia/Bot/network/board code is retained; Retro uses the shared device-tested
state machine rather than the old preview. Analyse/Setup remain their previous
placeholders, and real-board-to-native-Retro play is a later integration step.

Use the existing local PlatformIO core at `output/bauerino-pio-core` (relative
to the workspace). The post script reads the native project's generated CMake
compilation database and rebuilds its selected MAME sources against this
application's Arduino 3.3.5 / IDF 5.5 SDK. Configure the native `esp32s3` project
first if its generated files/database are absent. The compact dispatch and
native 8-bit LE bus profiles must be enabled for this hardware configuration.

Build `bauerino480`, then `buildfs` for its own LittleFS image. Use
`../tools/flash_bauerino_mame.py` to validate all images, then pass `--write`
to flash the guarded board. This helper writes the combined partition table,
firmware, assets and byte-verified ROM filesystem together; an application-only
upload does not install the required filesystems.

The current ROM Play screen is explicitly a test: choosing White inserts
e2e4 and requests an actual ROM reply; choosing Black lets the ROM open.
Move makes the engine play the next side. Load, UCI Options, Board, Notation,
Info and Quit are driven by the actual state machine. The top-left return
waits for clean emulator exit before restoring the app launcher.

USB diagnostics `tap x y`, `swipe -180`, `swipe 180`, `screenshot` and
`bots-test` support the device test at `../tools/test_bauerino_device.py`.
Screenshots contain the framebuffer painted on the panel; they are not panel
readback or camera photographs. Final finger/cold-start verification remains
a separate physical acceptance check.
