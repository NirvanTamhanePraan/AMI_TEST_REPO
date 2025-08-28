#include "dummy_leds.h"
#include "ble_main.h"

#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"

#define BLINK_GPIO 8

static uint8_t s_led_state = 0;

static led_strip_handle_t led_strip;

char local_rec_value5[32] = {0};

bool is_led_enabled = false;

int led_en1, log_en1;

int duty_val1;

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
}

static void configure_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void leds_task(void *pvParameters){
    /* Configure the peripheral according to the LED type */
    configure_led();

    while (1) {
        if (rec_value5 != NULL) {
            strncpy(local_rec_value5, rec_value5, sizeof(local_rec_value5) - 1);
            local_rec_value5[sizeof(local_rec_value5) - 1] = '\0';
            sscanf(local_rec_value5, "{CP:%d,%d,%d}\n",  &led_en1, &duty_val1, &log_en1);
        }

        if (led_en1 == 1){
            is_led_enabled = true;
            blink_led();
            s_led_state = !s_led_state;
            // printf("LED toggled. State: %s\n", s_led_state ? "ON" : "OFF");
        }

        if (led_en1 == 0){
            is_led_enabled = false;
            blink_led();
            // s_led_state = !s_led_state;
            // printf("LED toggled. State: %s\n", s_led_state ? "ON" : "OFF");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void leds_app_start(void) 
{
  xTaskCreate(&leds_task, "leds_task", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
}