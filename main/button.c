#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "button.h"
#include "lights.h"

static const char *TAG = "button";

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define DEBOUNCETIME (10)
static uint32_t cPresses;
static uint32_t cInvokes;
static uint32_t busy;

#define GPIO_INPUT_IO_0      (5)
// #define GPIO_INPUT_IO_1      (19)
// #define GPIO_INPUT_PIN_SEL   ((1ULL<<GPIO_INPUT_IO_0) | (1ULL<<GPIO_INPUT_IO_1))
#define GPIO_INPUT_PIN_SEL   (1ULL<<GPIO_INPUT_IO_0)
// #define GPIO_INPUT_PIN_SEL   (1ULL<<GPIO_INPUT_IO_1)
#define ESP_INTR_FLAG_DEFAULT (0)

static xQueueHandle gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    portENTER_CRITICAL_ISR(&mux);
    static uint32_t lastIsr;
    uint32_t now = xTaskGetTickCount();
    cPresses++;
    if ((busy == pdFALSE) && ((lastIsr == 0) || (now - lastIsr) > DEBOUNCETIME)) {
        uint32_t gpio_num = (uint32_t) arg;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
        lastIsr = now;
    }
    portEXIT_CRITICAL_ISR(&mux);
}

void fake_toggle() {
    portENTER_CRITICAL_ISR(&mux);
    cPresses++;
    if (busy == pdFALSE) {
        uint32_t gpio_num = (uint32_t) 1;
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
    portEXIT_CRITICAL_ISR(&mux);
}

static void do_the_thing(void* arg)
{
    uint32_t io_num;
    for(int i = 0; ;i++) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            cInvokes++;
            portENTER_CRITICAL_ISR(&mux);
            busy = pdTRUE;
            uint32_t presses = cPresses;
            portEXIT_CRITICAL_ISR(&mux);
            if ((i % 2) == 0) {
                ESP_LOGI(TAG, "Got a button press, setting access to read-write. %d/%d", cInvokes, presses);
                lights_8bit_color(255, 13, 13);
            } else {
                ESP_LOGI(TAG, "Got a button press, setting access to read-only. %d/%d", cInvokes, presses);
                lights_8bit_color(13, 255, 13);
            }
            lights_breathe(8, 300);
            if ((i % 2) == 0) {
                lights_8bit_color(255, 13, 13);
            } else {
                lights_8bit_color(13, 255, 13);
            }
            portENTER_CRITICAL_ISR(&mux);
            busy = pdFALSE;
            portEXIT_CRITICAL_ISR(&mux);
        }
    }
}

void button_init() {
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //change gpio intrrupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_POSEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(do_the_thing, "do_the_thing", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
    // //hook isr handler for specific gpio pin
    // gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

    //remove isr handler for gpio number.
    gpio_isr_handler_remove(GPIO_INPUT_IO_0);
    //hook isr handler for specific gpio pin again
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}

