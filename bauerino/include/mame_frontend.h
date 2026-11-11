#pragma once
#include <stdint.h>
bool mameFrontendBegin();
void mameFrontendTick();
bool mameFrontendBusy();
void mameFrontendStop();
bool mameFrontendDirty();
void mameFrontendDraw(uint16_t *framebuffer);
void mameFrontendTap(int x, int y);
void mameFrontendCapture(bool begin);
bool mameFrontendCacheMode(bool enabled);
bool mameFrontendSpeedMode(unsigned mode);
