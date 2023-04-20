#ifndef STOVECTRL_H
#define STOVECTRL_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct led_strip_s led_strip_t;

enum StoveCtrlMode
{
    SCM_Off,
    SCM_Manual,
    SCM_Timers
};

enum FanSpeed
{
    FS_Off,
    FS_Low,
    FS_High
};

extern void SC_init_gpio();
extern void SC_init(led_strip_t *led);

extern void SC_task_event();

extern void SC_set_target_temp(double temp);

extern esp_err_t get_state(httpd_req_t *req);
extern esp_err_t set_target_temperature(httpd_req_t *req);

extern esp_err_t set_light(httpd_req_t *req);
extern esp_err_t set_stove_mode(httpd_req_t *req);
extern esp_err_t set_cooling_fan(httpd_req_t *req);
extern esp_err_t set_downdraft_fan(httpd_req_t *req);
extern esp_err_t set_convection_fan(httpd_req_t *req);
extern esp_err_t set_use_top_element(httpd_req_t *req);
extern esp_err_t set_use_bottom_element(httpd_req_t *req);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // STOVECTRL_H
