
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <time.h>
#include <sys/time.h>

#include "led.h"
#include "i2c.h"
#include "wifi.h"
#include "httpd.h"
#include "mcp9600.h"
#include "stovectrl.h"
#include "cooktimers.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define SCL_PIN         15
#define SDA_PIN         14

#define LED_PIN          5

static i2c_port_t i2c_port = I2C_NUM_0;
static led_strip_t *strip = NULL;

static void initialize_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void sntp_sync_time(struct timeval *tv)
{
    settimeofday(tv, NULL);
    sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);

    time_t n = 0;
    struct tm now = { 0 };
    time(&n);
    localtime_r(&n, &now);

    printf("Time is synchronized to:\n");
    printf("\t%d-%d-%d, %d:%d:%d\n",
           now.tm_year, now.tm_mon, now.tm_mday,
           now.tm_hour, now.tm_min, now.tm_sec);
}

static void stove_control_task(void * parm)
{
    TickType_t last_wakeup = xTaskGetTickCount();
    while (1)
    {
        SC_task_event();
        vTaskDelayUntil(&last_wakeup, 50 / portTICK_PERIOD_MS);
    }
}


#define GPIO_INPUT_IO_0     0
#define GPIO_INPUT_PIN_SEL  (1ULL<<GPIO_INPUT_IO_0)
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void task_gpio_reset(void* arg)
{
    while(1)
    {
        uint32_t io_num;
        if(!xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
            continue;

        if(gpio_get_level(io_num) == 1)
            esp_restart();
    }
}

void init_gpio_reset()
{
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

    //change gpio interupt type for one pin
    gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(task_gpio_reset, "gpio_reset", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
}

void app_main(void)
{
    SC_init_gpio();

    init_gpio_reset();
    printf("Minimum free heap size: %ld bytes\n", esp_get_minimum_free_heap_size());

    printf("Bringing up NVS\n");
    initialize_nvs();

    //printf("Bringing up ESP Temp\n");
    //initalize_esp_temp();

    printf("Bringing up I2C\n");
    initalize_i2c(i2c_port, SCL_PIN, SDA_PIN, 100000);
    mcp9600_set_port(i2c_port);

    printf("Starting LEDs\n");
    strip = led_strip_init(0, LED_PIN, 1);

    SC_init(strip);
    CT_task_init();

    printf("Starting WiFi\n");

    if(wifi_init_sta() != ESP_OK)
    {
        // Must be handled better
        printf("FAILED TO START WiFi!\n");
    }

//    // We have internet credentials, so start the time service.
    //esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    //esp_sntp_setservername(0, "pool.ntp.org");
    //esp_sntp_init();
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

//    // TODO: Configurable Timezone
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();

    printf("WiFi started\n");
    strip->set_pixel(strip, 0, 0x255, 0x0, 0x0);
    strip->refresh(strip, 100);

    printf("Starting Webserver\n");
    register_webserver(0);

//   xTaskCreate(check_rssi, "rssi", 2048, NULL, 5, NULL);

    xTaskCreate(stove_control_task, "stove_ctrl", 2048, NULL, tskIDLE_PRIORITY+6, NULL);
}
