#include "mame_frontend.h"
#include <Arduino.h>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "../../esp32s3/src/retro_test_ui.h"
#include "../../esp32s3/src/bauerino_retro_lcd.h"

extern "C" {
int mame_s3_mount_roms(void);
int mame_s3_run_driver(const char *);
void retro_state_init(void);
void retro_state_tap(int,int);
bool retro_state_driver(char[4]);
bool retro_state_command(char *,unsigned);
void retro_state_open(const char *);
void retro_state_closed(void);
void retro_state_reply(const char *);
void retro_state_observe(const char *);
void retro_state_scanline(int,uint16_t *);
void retro_state_rate(float);
extern const uint8_t retro_art[] asm("_binary_retro_base_rgb565_start");
}
namespace {
SemaphoreHandle_t stateMutex;
QueueHandle_t loads;
TaskHandle_t worker;
alignas(16) StackType_t workerStack[32768/sizeof(StackType_t)];
StaticTask_t workerControl;
std::atomic<bool> busy{false}, stopRequested{false}, dirty{true};
struct Guard {
    Guard(){xSemaphoreTake(stateMutex,portMAX_DELAY);}
    ~Guard(){xSemaphoreGive(stateMutex);}
};
void engineTask(void *) {
    char driver[4];
    for(;;) {
        if(xQueueReceive(loads,driver,portMAX_DELAY)!=pdTRUE)continue;
        Serial.printf("[RETRO] load %s; internal=%u psram=%u\n",driver,
            unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
            unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
        // Keep internal RAM available for task stacks and network/DMA needs.
        // Ordinary MAME buffers use the now-available external heap instead.
        heap_caps_malloc_extmem_enable(0);
        if(mame_s3_mount_roms()) {
            int result=mame_s3_run_driver(driver);
            Serial.printf("[RETRO] exit %d; integrity=%s; psram=%u\n",result,
                heap_caps_check_integrity_all(true)?"OK":"FAILED",
                unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
        } else retro_ui_reply("info string error ROM-Dateien fehlen");
        retro_ui_closed();
        busy.store(false);stopRequested.store(false);dirty.store(true);
    }
}
}
bool mameFrontendBegin() {
    if(worker)return true;
    if(!stateMutex)stateMutex=xSemaphoreCreateMutex();
    if(!loads)loads=xQueueCreate(1,4);
    if(!stateMutex||!loads)return false;
    retro_ui_init();
    // Reserve the contiguous internal stack before Wi-Fi/BLE fragment that
    // heap. The worker remains asleep until the first actual Load request.
    worker=xTaskCreateStaticPinnedToCore(engineTask,"mame_uci",32768,nullptr,1,
        workerStack,&workerControl,1);
    if(!worker)return false;
    Serial.printf("[RETRO] worker reserved; internal=%u largest=%u\n",
        unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
        unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)));
    return true;
}
void mameFrontendTick() {
    if(!stateMutex||busy.load())return;
    char driver[4];
    if(!retro_ui_driver(driver))return;
    if(!worker&&!mameFrontendBegin()) {
        retro_ui_reply("info string error Kein RAM fuer MAME-Task");return;
    }
    busy.store(true);stopRequested.store(false);
    if(xQueueSend(loads,driver,0)!=pdTRUE){busy.store(false);retro_ui_reply("info string error Ladepuffer voll");}
}
bool mameFrontendBusy(){return busy.load();}
void mameFrontendStop(){if(busy.load())stopRequested.store(true);}
bool mameFrontendDirty(){return dirty.exchange(false);}
void mameFrontendDraw(uint16_t *pixels) {
    if(!stateMutex)return;
    Guard lock;
    for(int y=0;y<480;++y) {
        memcpy(pixels+y*480,retro_art+y*960,960);
        retro_state_scanline(y,pixels+y*480);
    }
}
void mameFrontendTap(int x,int y){if(stateMutex)retro_ui_tap(x,y);}
void mameFrontendCapture(bool begin) {
    if(begin){xSemaphoreTake(stateMutex,portMAX_DELAY);flockfile(stdout);flockfile(stderr);}
    else {funlockfile(stderr);funlockfile(stdout);xSemaphoreGive(stateMutex);}
}

extern "C" {
void retro_ui_init(){Guard lock;retro_state_init();}
void retro_ui_poll(){} // Existing frontend owns physical touch and navigation.
void retro_ui_tap(int x,int y){Guard lock;retro_state_tap(x,y);}
bool retro_ui_driver(char name[4]){Guard lock;return retro_state_driver(name);}
bool retro_ui_command(char *line,unsigned capacity){
    if(stopRequested.exchange(false)) {
        if(capacity<5){stopRequested.store(true);return false;}
        Guard lock;retro_state_closed();
        memcpy(line,"quit",5);return true;
    }
    Guard lock;return retro_state_command(line,capacity);
}
void retro_ui_open(const char *name){Guard lock;retro_state_open(name);Serial.printf("[RETRO OPEN] %s\n",name);}
void retro_ui_closed(){Guard lock;retro_state_closed();}
void retro_ui_reply(const char *line){Guard lock;retro_state_reply(line);Serial.printf("[RETRO UCI] %s\n",line);}
void retro_ui_observe(const char *line){Guard lock;retro_state_observe(line);}
void retro_ui_scanline(int y,uint16_t *pixels){Guard lock;retro_state_scanline(y,pixels);}
void retro_ui_rate(float rate){Guard lock;retro_state_rate(rate);}
bool retro_touch_init(){return false;}
bool retro_touch_point(int *,int *){return false;}
bool bauerino_lcd_init(){return true;}
bool bauerino_lcd_online(){return true;}
void bauerino_lcd_invalidate(bool){dirty.store(true);}
void bauerino_lcd_driver(const char *){dirty.store(true);}
void bauerino_lcd_digits(const uint8_t[4]){}
void bauerino_lcd_uci(const char *){}
void bauerino_lcd_flush(){}
void bauerino_lcd_capture(){Serial.println("[RETRO] Use frontend screen capture");}
}
