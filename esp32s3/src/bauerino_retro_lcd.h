#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool bauerino_lcd_init(void);
bool bauerino_lcd_online(void);
void bauerino_lcd_invalidate(bool full);
void bauerino_lcd_driver(const char *name);
void bauerino_lcd_digits(const uint8_t values[4]);
void bauerino_lcd_uci(const char *line);
void bauerino_lcd_flush(void);
void bauerino_lcd_capture(void);
#ifdef __cplusplus
}
#endif
