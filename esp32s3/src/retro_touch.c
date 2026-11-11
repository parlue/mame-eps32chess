// CST9220 protocol follows SensorLib TouchDrvCST92xx (Lewis He, MIT).
// Native IDF transport; no Arduino runtime or task/interrupt allocations.
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include <stdio.h>
#include <stdbool.h>
static i2c_master_dev_handle_t device;
static int64_t next_poll;
static bool previous;
static int last_x,last_y;
static volatile uint32_t events=1;
static uint32_t processed_events;
static void IRAM_ATTR touch_interrupt(void *argument) {(void)argument;++events;}
static esp_err_t reg_read(uint16_t reg,void *data,unsigned length) {
    uint8_t address[2]={reg>>8,reg&255};
    // Arduino SensorCommI2C uses STOP between address write and data read.
    esp_err_t error=i2c_master_transmit(device,address,2,20);
    return error==ESP_OK?i2c_master_receive(device,data,length,20):error;
}
bool retro_touch_init(void) {
    i2c_master_bus_config_t config={.i2c_port=I2C_NUM_0,.sda_io_num=15,.scl_io_num=14,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
    i2c_master_bus_handle_t bus;
    esp_err_t error=i2c_new_master_bus(&config,&bus);
    i2c_device_config_t dev={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=0x5a,.scl_speed_hz=400000};
    if(error==ESP_OK)error=i2c_master_bus_add_device(bus,&dev,&device);
    if(error!=ESP_OK){printf("[TOUCH ERROR] init %d\n",error);return false;}
    gpio_config_t reset={.pin_bit_mask=1ULL<<40,.mode=GPIO_MODE_OUTPUT};
    error=gpio_config(&reset);
    if(error!=ESP_OK){printf("[TOUCH ERROR] reset GPIO %d\n",error);return false;}
    gpio_set_level(40,0);vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(40,1);vTaskDelay(pdMS_TO_TICKS(40));
    gpio_set_direction(11,GPIO_MODE_INPUT);gpio_set_pull_mode(11,GPIO_PULLUP_ONLY);
    uint8_t command[]={0xd1,0x01};
    error=i2c_master_transmit(device,command,2,20);vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t info[4]={0};if(error==ESP_OK)error=reg_read(0xd1fc,info,4);
    bool valid=error==ESP_OK&&info[3]==0xca&&info[2]==0xca;
    uint8_t normal[]={0xd1,0x09};
    if(valid)valid=i2c_master_transmit(device,normal,2,20)==ESP_OK;
    if(valid){
        esp_err_t irq=gpio_install_isr_service(0);
        if(irq!=ESP_OK&&irq!=ESP_ERR_INVALID_STATE)valid=false;
        if(valid)valid=gpio_set_intr_type(11,GPIO_INTR_NEGEDGE)==ESP_OK&&
            gpio_isr_handler_add(11,touch_interrupt,NULL)==ESP_OK;
    }
    printf("[TOUCH %s] CST92xx code=%02x%02x%02x%02x\n",valid?"READY":"ERROR",info[3],info[2],info[1],info[0]);
    return valid;
}
bool retro_touch_point(int *x,int *y) {
    int64_t now=esp_timer_get_time();
    if(now<next_poll){*x=last_x;*y=last_y;return previous;}
    next_poll=now+30000;
    uint32_t pending=events;
    if(pending==processed_events&&gpio_get_level(11)){
        *x=last_x;*y=last_y;return previous;
    }
    processed_events=pending;
    // CST92XX_MAX_FINGER_NUM is TWO: 2*5+5 = 15 bytes, matching SensorLib.
    uint8_t data[15]={0};
    esp_err_t error=reg_read(0xd000,data,sizeof(data));
    uint8_t ack[]={0xd0,0x00,0xab};
    if(error==ESP_OK)error=i2c_master_transmit(device,ack,3,20);
    bool pressed=error==ESP_OK&&data[6]==0xab&&(data[5]&0x7f)>0&&(data[0]&15)==6;
    if(pressed){int raw_x=(data[1]<<4)|(data[3]>>4),raw_y=(data[2]<<4)|(data[3]&15);
        // Same swap/mirror transform as the working Arduino display frontend.
        last_x=480-raw_y;last_y=raw_x;
        if(last_x<0)last_x=0;
        if(last_x>479)last_x=479;
        if(last_y<0)last_y=0;
        if(last_y>479)last_y=479;
        if(!previous)printf("[TOUCH POINT] raw=%d,%d ui=%d,%d\n",raw_x,raw_y,last_x,last_y);
    }
    *x=last_x;*y=last_y;previous=pressed;return pressed;
}
