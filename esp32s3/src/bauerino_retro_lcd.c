// Waveshare 2.16 CO5300: original Retro artwork, live ROM digits and UCI move.
// One 960-byte DMA scanline; no full framebuffer and no Arduino dependency.
// Pinout/initialisation match the previously working Bauerino display driver.
#include "bauerino_retro_lcd.h"
#include "retro_test_ui.h"
#include "lcd_font.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern const uint8_t base_start[] asm("_binary_retro_base_rgb565_start");
static spi_device_handle_t panel;
static uint16_t *row;
static bool online, dirty, full_dirty;
static uint8_t rom_digits[4];
static char last_move[6] = "----", model[24] = "MAME 0.220";
static int64_t last_flush;

static bool command(uint8_t reg, const void *data, size_t size)
{
    spi_transaction_t t = {.cmd=0x02, .addr=(uint32_t)reg<<8,
        .length=size*8, .tx_buffer=data,
        .flags=SPI_TRANS_MULTILINE_CMD|SPI_TRANS_MULTILINE_ADDR};
    gpio_set_level(12,0);
    esp_err_t result=spi_device_polling_transmit(panel,&t);
    gpio_set_level(12,1);
    return result==ESP_OK;
}
static bool value(uint8_t reg,uint8_t data) { return command(reg,&data,1); }

static void rect(int y,int x,int top,int w,int h,uint16_t color,int radius)
{
    if(y<top || y>=top+h)return;
    int dy=y-top, cut=0;
    if(radius && (dy<radius || dy>=h-radius)) {
        int v=dy<radius?radius-dy:radius-(h-1-dy), dx=0;
        while((dx+1)*(dx+1)+v*v<=radius*radius)++dx;
        cut=radius-dx;
    }
    int begin=x+cut,end=x+w-cut;
    if(begin<0)begin=0;
    if(end>480)end=480;
    for(int i=begin;i<end;++i)row[i]=color;
}
static void segment(int scan,int x,int y,int w,int h,bool lit)
{
    rect(scan,x-2,y-2,w+4,h+4,lit?0x3800:0x1000,4);
    rect(scan,x,y,w,h,lit?0xf800:0x1800,3);
    if(lit)rect(scan,x+2,y+2,w-4,h-4,0xf980,2);
}
static void digit(int scan,int x,int y,uint8_t mask)
{
    segment(scan,x+9,y,44,7,mask&1);segment(scan,x+54,y+8,7,30,mask&2);
    segment(scan,x+54,y+48,7,30,mask&4);segment(scan,x+9,y+79,44,7,mask&8);
    segment(scan,x+1,y+48,7,30,mask&16);segment(scan,x+1,y+8,7,30,mask&32);
    segment(scan,x+9,y+40,44,7,mask&64);
    if(mask&128)rect(scan,x+64,y+80,4,4,0xf800,2);
}
static uint8_t letter(char c)
{
    static const uint8_t numbers[]={0x3f,6,0x5b,0x4f,0x66,0x6d,0x7d,7,0x7f,0x6f};
    if(c>='0'&&c<='9')return numbers[c-'0'];
    switch(c){case 'a':return 0x77;case 'b':return 0x7c;case 'c':return 0x58;
        case 'd':return 0x5e;case 'e':return 0x79;case 'f':return 0x71;
        case 'g':return 0x6f;case 'h':return 0x74;default:return 0x40;}
}
static void text(int scan,int x,int y,const char *s,uint16_t color)
{
    if(scan<y||scan>=y+8)return;
    int bit=scan-y;
    for(;*s;++s,x+=6)for(int i=0;i<5;++i)
        if(x+i>=0 && x+i<480 && (font[(uint8_t)*s*5+i]&(1<<bit)))row[x+i]=color;
}
static void compose(int y)
{
    memcpy(row,base_start+y*960,960);
    if(y>=70&&y<260) {
        rect(y,76,70,340,190,0x0800,0);
        for(int i=0;i<4;++i) {
            int x=84+i*76+(i>=2?18:0);
            digit(y,x,72,rom_digits[i]);
            digit(y,x,171,letter(last_move[i]));
        }
        text(y,240-(int)strlen(model)*3,161,model,0xe6b3);
    }
    if(y>=269&&y<286) {
        rect(y,42,269,396,17,0,0);
        char caption[40];snprintf(caption,sizeof(caption),"Letzter ROM-Zug: %s",last_move);
        text(y,240-(int)strlen(caption)*3,274,caption,0xe6b3);
    }
    retro_ui_scanline(y,row);
}
static bool send_row(int y)
{
    // CO5300 windows follow the vendor's even-start/odd-end alignment.
    // Keep CS asserted across the two scanlines and send the second as raw
    // QSPI continuation, exactly like Arduino_ESP32QSPI::writePixels chunks.
    bool first=(y&1)==0;
    if(first){
        uint8_t x[]={0,0,1,0xdf},v[]={y>>8,y&255,(y+1)>>8,(y+1)&255};
        if(!command(0x2a,x,4)||!command(0x2b,v,4)||!command(0x2c,NULL,0))return false;
        gpio_set_level(12,0);
    }
    for(int i=0;i<480;++i)row[i]=(row[i]>>8)|(row[i]<<8);
    spi_transaction_ext_t t={.base={.cmd=0x32,.addr=0x003c00,
        .flags=SPI_TRANS_MODE_QIO,.length=960*8,.tx_buffer=row}};
    if(!first)t.base.flags|=SPI_TRANS_VARIABLE_CMD|SPI_TRANS_VARIABLE_ADDR|SPI_TRANS_VARIABLE_DUMMY;
    esp_err_t result=spi_device_polling_transmit(panel,&t.base);
    if(!first||result!=ESP_OK)gpio_set_level(12,1);
    return result==ESP_OK;
}
static bool paint(int first,int end)
{
    for(int y=first;y<end;++y){
        compose(y);if(!send_row(y))return false;
        if(!(y&15))vTaskDelay(1);
    }
    return true;
}
bool bauerino_lcd_init(void)
{
    spi_bus_config_t bus={.mosi_io_num=4,.miso_io_num=5,.sclk_io_num=38,
        .quadwp_io_num=6,.quadhd_io_num=7,.data4_io_num=-1,.data5_io_num=-1,
        .data6_io_num=-1,.data7_io_num=-1,.max_transfer_sz=1024,
        .flags=SPICOMMON_BUSFLAG_MASTER|SPICOMMON_BUSFLAG_GPIO_PINS};
    spi_device_interface_config_t dev={.command_bits=8,.address_bits=24,
        .mode=0,.clock_speed_hz=40000000,.spics_io_num=-1,
        .flags=SPI_DEVICE_HALFDUPLEX,.queue_size=1};
    // set_direction alone does not select the GPIO IO-mux function in IDF.
    // In particular GPIO39 is also MTCK: explicitly reclaim reset from JTAG.
    gpio_config_t outputs={.pin_bit_mask=(1ULL<<12)|(1ULL<<39),.mode=GPIO_MODE_OUTPUT};
    esp_err_t pins=gpio_config(&outputs);
    if(pins!=ESP_OK){printf("[LCD ERROR] GPIO config: %d\n",pins);return false;}
    gpio_set_level(12,1);gpio_set_level(39,1);
    esp_err_t error=spi_bus_initialize(SPI2_HOST,&bus,SPI_DMA_CH_AUTO);
    if(error==ESP_OK)error=spi_bus_add_device(SPI2_HOST,&dev,&panel);
    if(error!=ESP_OK){printf("[LCD ERROR] SPI init: %d\n",error);return false;}
    error=spi_device_acquire_bus(panel,portMAX_DELAY);
    if(error!=ESP_OK){printf("[LCD ERROR] SPI acquire: %d\n",error);return false;}
    row=heap_caps_malloc(960,MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA);
    if(!row){printf("[LCD ERROR] scanline allocation\n");return false;}
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(39,0);vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(39,1);vTaskDelay(pdMS_TO_TICKS(200));
    bool ok=command(0x11,NULL,0);vTaskDelay(pdMS_TO_TICKS(120));
    ok=ok&&value(0xfe,0)&&value(0xc4,0x80)&&value(0x3a,0x55)&&value(0x53,0x20);
    ok=ok&&value(0x63,0xff)&&command(0x29,NULL,0)&&value(0x51,160)&&value(0x58,0);
    ok=ok&&command(0x20,NULL,0)&&value(0x36,0xa0);
    vTaskDelay(pdMS_TO_TICKS(10));
    online=ok&&paint(0,480);
    printf("[LCD %s] Waveshare CO5300 480x480; 960-byte scanline\n",online?"READY":"ERROR");
    return online;
}
void bauerino_lcd_driver(const char *name)
{
    snprintf(model,sizeof(model),"Mephisto %s",!strcmp(name,"mm5")?"MM V":"MM IV");
    memset(rom_digits,0,sizeof(rom_digits));memcpy(last_move,"----",5);dirty=true;
}
void bauerino_lcd_digits(const uint8_t values[4])
{
    if(memcmp(rom_digits,values,4)){memcpy(rom_digits,values,4);dirty=true;}
}
void bauerino_lcd_uci(const char *line)
{
    if(!strncmp(line,"bestmove ",9)&&strlen(line)>=13){memcpy(last_move,line+9,4);last_move[4]=0;dirty=true;}
}
void bauerino_lcd_flush(void)
{
    if(!online||!dirty||esp_timer_get_time()-last_flush<250000)return;
    if(!paint(full_dirty?0:70,full_dirty?480:286)){printf("[LCD ERROR] update failed\n");online=false;return;}
    last_flush=esp_timer_get_time();dirty=false;full_dirty=false;
}
bool bauerino_lcd_online(void) {return online;}
void bauerino_lcd_invalidate(bool full) {dirty=true;full_dirty=full_dirty||full;}
void bauerino_lcd_capture(void)
{
    if(!online){printf("[LCD ERROR] capture unavailable\n");return;}
    // VFS defaults to CRLF expansion; binary pixels must pass unchanged.
    usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_LF);
    // Repaint and transmit the exact same scanlines sent to the physical panel.
    printf("[LCD FRAME] 480 480 RGB565BE 460800\n");fflush(stdout);
    int64_t deadline=esp_timer_get_time()+45000000;
    for(int y=0;y<480;++y){
        compose(y);if(!send_row(y))break;
        size_t offset=0;
        while(offset<960){ssize_t n=write(STDOUT_FILENO,(uint8_t*)row+offset,960-offset);
            if(n>0)offset+=n;else vTaskDelay(1);
            if(esp_timer_get_time()>deadline){printf("\n[LCD ERROR] capture timeout\n");return;}
        }
        if(!(y&7))vTaskDelay(1);
    }
    printf("\n[LCD FRAME END] lastmove=%s\n",last_move);fflush(stdout);
    dirty=false;
}
