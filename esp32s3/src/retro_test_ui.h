#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void retro_ui_init(void);
void retro_ui_poll(void);
void retro_ui_tap(int x, int y);
bool retro_ui_driver(char name[4]);
bool retro_ui_command(char *line, unsigned capacity);
void retro_ui_open(const char *driver);
void retro_ui_closed(void);
void retro_ui_reply(const char *line);
void retro_ui_observe(const char *line);
void retro_ui_scanline(int y, uint16_t *pixels);
void retro_ui_rate(float percent);
#ifdef __cplusplus
}
#endif
