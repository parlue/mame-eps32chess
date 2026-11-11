#include "bauerino_retro_lcd.h"
#include <stdio.h>

/* Core stability firmware: the optional native LCD is intentionally deferred. */
bool bauerino_lcd_init(void) { return false; }
void bauerino_lcd_driver(const char *name) { (void)name; }
void bauerino_lcd_digits(const uint8_t values[4]) { (void)values; }
void bauerino_lcd_uci(const char *line) { (void)line; }
void bauerino_lcd_flush(void) {}
void bauerino_lcd_capture(void) { puts("[LCD DISABLED] core stability firmware"); }
