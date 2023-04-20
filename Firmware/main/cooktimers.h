#ifndef COOKTIMERS_H
#define COOKTIMERS_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void CT_task_init();
extern void CT_update();

extern esp_err_t get_timers(httpd_req_t *req);
extern esp_err_t add_timer(httpd_req_t *req);
extern esp_err_t rm_timer(httpd_req_t *req);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // COOKTIMERS_H
