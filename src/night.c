#include "night.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "config/app_config.h"
#include "hidemo.h"

#include "gpio.h"

#define tag "[night]: "

static bool night_mode = false;

bool night_mode_is_enable() {
    return night_mode;
}

void ircut_on() {
    set_pin_linux(app_config.ir_cut_pin1, false);
    set_pin_linux(app_config.ir_cut_pin2, true);
    // usleep(app_config.pin_switch_delay_us);
    // set_pin_linux(app_config.ir_cut_pin1, false);
    // set_pin_linux(app_config.ir_cut_pin2, false);
    set_color2gray(true);
}

void ircut_off() {
    set_pin_linux(app_config.ir_cut_pin1, true);
    set_pin_linux(app_config.ir_cut_pin2, false);
    // usleep(app_config.pin_switch_delay_us);
    // set_pin_linux(app_config.ir_cut_pin1, false);
    // set_pin_linux(app_config.ir_cut_pin2, false);
    set_color2gray(false);
}

void set_night_mode(bool night) {
    if (night) {
        printf("Change mode to NIGHT\n");
        ircut_off();
        // set_color2gray(true);
    } else {
        printf("Change mode to DAY\n");
        ircut_on();
        // set_color2gray(false);
    }
}

extern bool keepRunning;

void *thread_ipcam_pwm(void *vargp)
{
    int pwm_num = 0, direction = 0;
    while(1){
        if(!direction){
            pwm_num += 50;
            if(pwm_num >= 1000){
                direction = 1;
            }
        }
        else
        {
            pwm_num -= 50;
            if(pwm_num <= 0){
                direction = 0;
            }
        }
        hi3518_pwm_set(pwm_num);
        usleep(100000);
    }
}

void* night_thread_func(void *vargp)  
{
    pthread_t tid_ipcam_led;
    int adc_value = 0, night_mode = 0, rblink = 0, ret = 0;
    usleep(1000);
    hi3518_adc_init();
    printf("Change mode to DAY\n");
    ircut_off();

    ret = pthread_create(&tid_ipcam_led , NULL , thread_ipcam_pwm , NULL);
    if(ret){
        perror("thread_ipcam_pwm creat error!\r\n");
    }
    while (keepRunning)
    {
        adc_value = hi3518_adc_get();
        if(adc_value > app_config.ir_sensor_threshold){//ir_sensor_threshold
            if(night_mode){
                printf("Change mode to DAY\n");
                ircut_off();
            }
            night_mode = 0;
        }
        else
        {
            if(!night_mode){
                printf("Change mode to NIGHT\n");
                ircut_on();
            }
            night_mode = 1;
        }
        // printf("hi3518_adc_get-> %d\r\n", adc_value);
        sleep(app_config.check_interval_s);
    }
}

int32_t start_monitor_light_sensor() {
    pthread_t thread_id = 0;

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    size_t stacksize;
    pthread_attr_getstacksize(&thread_attr,&stacksize);
    size_t new_stacksize = 16*1024;
    if (pthread_attr_setstacksize(&thread_attr, new_stacksize)) { printf(tag "Error:  Can't set stack size %ld\n", new_stacksize); }
    pthread_create(&thread_id, &thread_attr, night_thread_func, NULL);
    if (pthread_attr_setstacksize(&thread_attr, stacksize)) { printf(tag "Error:  Can't set stack size %ld\n", stacksize); }
    pthread_attr_destroy(&thread_attr);
}
