#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "lights.h"

// Not MT Safe
static int duties[3];
 
#define LED_R_GPIO   (21)
#define LED_G_GPIO   (4)
#define LED_B_GPIO   (18)
#define LED_R_CH     (LEDC_CHANNEL_0)
#define LED_G_CH     (LEDC_CHANNEL_1)
#define LED_B_CH     (LEDC_CHANNEL_2)

#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<LED_B_GPIO) | (1ULL<<LED_R_GPIO) | (1ULL<<LED_G_GPIO))

#define LEDC_NUM_CH       (3)
#define LEDC_HS_TIMER_R   (LEDC_TIMER_0)
#define LEDC_HS_TIMER_G   (LEDC_TIMER_1)
#define LEDC_HS_TIMER_B   (LEDC_TIMER_2)
#define LEDC_HS_MODE      (LEDC_HIGH_SPEED_MODE)

static void _set_by_duties();

/*
* Prepare and set configuration of timers
* that will be used by LED Controller
*/
static ledc_timer_config_t ledc_timer[LEDC_NUM_CH] = {
    {
        .duty_resolution = LEDC_TIMER_8_BIT,  // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER_R,           // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    },
    {
        .duty_resolution = LEDC_TIMER_8_BIT,  // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER_G,           // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    },
    {
        .duty_resolution = LEDC_TIMER_8_BIT,  // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HS_MODE,           // timer mode
        .timer_num = LEDC_HS_TIMER_B,           // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    }
};

static ledc_channel_config_t ledc_channel[LEDC_NUM_CH] = {
    {
        .channel    = LED_R_CH,
        .duty       = COLOR_MAX,
        .gpio_num   = LED_R_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER_R
    },
    {
        .channel    = LED_G_CH,
        .duty       = COLOR_MAX,
        .gpio_num   = LED_G_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER_G
    },
    {
        .channel    = LED_B_CH,
        .duty       = COLOR_MAX,
        .gpio_num   = LED_B_GPIO,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_HS_TIMER_B
    }
};


void lights_init() {
     gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    lights_basic_color(LED_OFF, LED_OFF, LED_OFF);
    duties[0] = NORM_COLOR(0);
    duties[1] = NORM_COLOR(0);
    duties[2] = NORM_COLOR(0);

    // Set LED Controller with previously prepared configuration
    for (int ch = 0; ch < LEDC_NUM_CH; ch++) {
        ledc_timer_config(&ledc_timer[ch]);
        ledc_channel_config(&ledc_channel[ch]);
    }

    // Initialize fade service.
    ledc_fade_func_install(0);
}

void lights_basic_color(int r, int g, int b) {
    gpio_set_level(LED_R_GPIO, r != LED_ON);
    gpio_set_level(LED_G_GPIO, g != LED_ON);
    gpio_set_level(LED_B_GPIO, b != LED_ON);
}

static void _set_by_duties() {
    for (int ch = 0; ch < LEDC_NUM_CH; ch++) {
        ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, duties[ch]);
        ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
    }
}

void lights_8bit_color(int r, int g, int b) {
    duties[0] = NORM_COLOR(r);
    duties[1] = NORM_COLOR(g);
    duties[2] = NORM_COLOR(b);
    _set_by_duties();
}

void lights_breathe(int cycles, int dur_ms) {
    for (int i = 0; i < cycles; i++) {
        for (int ch = 0; ch < LEDC_NUM_CH; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, NORM_COLOR(0), dur_ms);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }
        vTaskDelay(dur_ms / portTICK_PERIOD_MS);

        for (int ch = 0; ch < LEDC_NUM_CH; ch++) {
            ledc_set_fade_with_time(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, duties[ch], dur_ms);
            ledc_fade_start(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_FADE_NO_WAIT);
        }
        vTaskDelay(dur_ms / portTICK_PERIOD_MS);
    }
    _set_by_duties();
}