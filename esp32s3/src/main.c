// MAME 0.220 ROM bring-up on the Waveshare ESP32-S3.
// This image tests the real core and ROM; the Bauerino UI is a later step.

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_flash.h"
#include "esp_debug_helpers.h"
#include "esp_rom_sys.h"
#include "bauerino_retro_lcd.h"
#include "retro_test_ui.h"

#define TAG "mame_s3_skeleton"

// Aus der MAME-0.220-Komponente (components/mame0220/src/mame_bridge.cpp)
extern int mame_bridge_selfcheck(void);
extern const char *mame_bridge_note(void);

// Startpfad der Emulation (components/mame0220/src/s3_startup.cpp).
// mame_s3_target() nennt Treiber und Bauversion, mame_s3_run() baut die
// Maschine mm4 auf und laesst sie laufen.
extern const char *mame_s3_target(void);
extern int mame_s3_run(void);
extern int mame_s3_run_driver(const char *name);
extern int mame_s3_mount_roms(void);

// Provisional memory profile (must be confirmed on real hardware, see docs/07).
#define EST_ROM_MM4_BYTES   (32u * 1024u)   // mm4: 0x8000 bytes at 0x8000
#define EST_ADDR_SPACE      (64u * 1024u)   // 65C02 address space
#define TARGET_CPU_HZ       4915200u        // 9.8304 MHz XTAL / 2

static void report_memory(void)
{
    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    printf("--- memory ---\n");
    printf("flash:                 %u bytes\n", (unsigned)flash_size);
    printf("heap internal free:    %u bytes\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("heap internal largest: %u bytes\n",
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("heap SPIRAM free:      %u bytes\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("heap SPIRAM largest:   %u bytes\n",
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    printf("heap DMA free:         %u bytes\n",
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA));
    printf("----------------\n");
}

static void report_allocation_failure(size_t bytes, uint32_t caps, const char *function)
{
    // A failing std::vector allocation is otherwise reduced to bad_alloc by
    // MAME's exception handler. Preserve the actual request and call site.
    esp_rom_printf("[ALLOC FAIL] bytes=%u caps=0x%x function=%s internal=%u/%u psram=%u/%u\n",
        (unsigned)bytes, (unsigned)caps, function,
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    esp_backtrace_print(12);
}

static bool report_heap_block(walker_heap_into_t heap, walker_block_info_t block, void *context)
{
    (void)heap; (void)context;
    if (block.used && block.size <= 256) {
        // ROM output avoids allocator use while the heap walker holds its lock.
        const unsigned char *data = block.ptr;
        char hex[97];
        const char digits[] = "0123456789abcdef";
        size_t bytes = block.size < 48 ? block.size : 48;
        for (size_t i = 0; i < bytes; ++i) {
            hex[2*i] = digits[data[i] >> 4];
            hex[2*i+1] = digits[data[i] & 15];
        }
        hex[2*bytes] = 0;
        esp_rom_printf("[HEAP BLOCK] %p %u %s\n", block.ptr, (unsigned)block.size, hex);
    }
    return true;
}

static void select_driver(char driver[4], bool initial)
{
    char line[64];
    size_t length = 0;
    bool overflow = false;
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    printf("[LOAD READY] load mm4 | load mm5%s\n", initial ? "; default mm4 in 3 seconds" : "");
    const int64_t deadline = esp_timer_get_time() + 3000000;
    for (;;) {
        retro_ui_poll();
        bauerino_lcd_flush();
        if (retro_ui_driver(driver)) {
            printf("[LOAD] %s (touch)\n",driver);
            return;
        }
        char input[64];
        ssize_t count = read(STDIN_FILENO, input, sizeof(input));
        for (ssize_t i = 0; i < count; ++i) {
            char c = input[i];
            if (c == '\r' || c == '\n') {
                line[length] = 0;
                int tap_x,tap_y;
                if (!overflow && sscanf(line,"tap %d %d",&tap_x,&tap_y)==2) {
                    retro_ui_tap(tap_x,tap_y);length=0;continue;
                }
                if (!overflow && !strcmp(line,"screenshot")) {
                    bauerino_lcd_capture();length=0;continue;
                }
                if (!overflow && !strcmp(line, "heap")) {
                    esp_rom_printf("[HEAP BEGIN]\n");
                    heap_caps_walk(MALLOC_CAP_INTERNAL, report_heap_block, NULL);
                    esp_rom_printf("[HEAP END]\n");
                    length = 0;
                    continue;
                }
                if (!overflow && (!strcmp(line, "load mm4") || !strcmp(line, "load mm5"))) {
                    memcpy(driver, line + 5, 4);
                    printf("[LOAD] %s\n", driver);
                    return;
                }
                if (length || overflow) printf("[LOAD ERROR] expected load mm4 or load mm5\n");
                length = 0;
                overflow = false;
            } else if (!overflow) {
                if (length < sizeof(line) - 1) line[length++] = c;
                else { length = 0; overflow = true; }
            }
        }
        if (initial && !bauerino_lcd_online() && esp_timer_get_time() >= deadline) {
            memcpy(driver, "mm4", 4);
            printf("[LOAD] mm4 (default)\n");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    heap_caps_register_failed_alloc_callback(report_allocation_failure);
    esp_chip_info_t info;
    esp_chip_info(&info);
    printf("\n=== MAME 0.220 ESP32-S3 ROM bring-up ===\n");
    printf("chip: %s, cores=%d, revision=%d\n",
           CONFIG_IDF_TARGET, info.cores, info.revision);
    printf("idf:  %s\n", esp_get_idf_version());
    printf("mm4 rom estimate: %u bytes at 0x8000\n", (unsigned)EST_ROM_MM4_BYTES);
    report_memory();

    bauerino_lcd_init();
    retro_ui_init();

    // MAME-0.220-Komponente wirklich aufrufen (sonst wird sie wegoptimiert)
    printf("mame: %s\n", mame_bridge_note());
    printf("mame selfcheck: %s\n", mame_bridge_selfcheck() ? "OK" : "FEHLER");

    if (!mame_s3_mount_roms()) {
        printf("ROM filesystem unavailable; MAME not started\n");
        return;
    }

    // Meilenstein 1b/1c: Kern, CPU und Treiber mm4 aufbauen und laufen lassen.
    // mame_s3_run() liefert erst zurueck, wenn die Maschine beendet ist.
    printf("mame ziel: %s\n", mame_s3_target());
    bool initial = true;
    for (;;) {
        char driver[4];
        select_driver(driver, initial);
        initial = false;
        bauerino_lcd_driver(driver);
        bauerino_lcd_flush();
        int mame_result = mame_s3_run_driver(driver);
        printf("mame lauf beendet: %d\n", mame_result);
        bool heap_ok = heap_caps_check_integrity_all(true);
        printf("[MAME IDLE] psram=%u internal=%u integrity=%s\n",
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               heap_ok ? "OK" : "FAILED");
        report_memory();
    }
}
