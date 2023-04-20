/* Scan Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
    This example shows how to scan for available set of APs.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

static const char *TAG = "scan";

/* Initialize Wi-Fi as sta and set scan method */
char *wifi_scan(size_t *len)
{
//    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//    ESP_ERROR_CHECK(esp_wifi_start());

    int err = esp_wifi_scan_start(NULL, true);
    if(err)
    {
        switch(err)
        {
            case ESP_ERR_WIFI_NOT_INIT:    printf("Scan Failed! ESP_ERR_WIFI_NOT_INIT\n"); break;
            case ESP_ERR_WIFI_NOT_STARTED: printf("Scan Failed! ESP_ERR_WIFI_NOT_STARTED\n"); break;
            case ESP_ERR_WIFI_TIMEOUT:     printf("Scan Failed! ESP_ERR_WIFI_TIMEOUT\n"); break;
            case ESP_ERR_WIFI_STATE:       printf("Scan Failed! ESP_ERR_WIFI_STATE\n"); break;
            default: printf("Scan Failed! %i\n", err); break;
        }
        return NULL;
    }

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

    if(ap_count == 0)
    {
        printf("No access points!\n");
        return NULL;
    }

    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_count);
    memset(ap_info, 0, sizeof(wifi_ap_record_t) * ap_count);

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);

    size_t totalLen = 1;
    for (int i = 0; i < ap_count; i++)
    {
        ESP_LOGI(TAG, "SSID \t\t%s\t%i", ap_info[i].ssid, strlen((char*)ap_info[i].ssid));
        totalLen += strlen((char*)ap_info[i].ssid) + 1;
    }

    if(ap_count > 1)
        totalLen--;

    char *result = malloc(totalLen);
    if(!result)
    {
        printf("Out of memory!\n");
        return NULL;
    }

    char *ptr = result;
    for (int i = 0; i < ap_count; i++)
    {
        strcpy(ptr, (char*)ap_info[i].ssid);
        ptr += strlen((char*)ap_info[i].ssid);
        *ptr = '\n';
        ptr++;
    }
    ptr--;
    *ptr = 0;

    if(len)
        *len = totalLen - 1;

    return result;
}
