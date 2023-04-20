#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int register_webserver();
extern esp_err_t get_content(httpd_req_t *req, char *content, size_t content_size);
extern esp_err_t ack_http_post(httpd_req_t *req);

#ifdef __cplusplus
} // extern "C"
#endif
