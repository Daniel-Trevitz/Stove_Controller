#include "httpd.h"
#include "mcp9600.h"
#include "cooktimers.h"
#include "stovectrl.h"
#include "urldecode.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

static httpd_handle_t server = NULL;

#define CONCAT2(a,b) a ## b

#define HTTP_CHECK(url, type, name) \
    if (0 == strncmp(req->uri, url, sizeof(url))) { \
        httpd_resp_set_type(req, type); \
        extern const char CONCAT2(name, _start)[] asm("_binary_" #name "_start"); \
        extern const uint16_t CONCAT2(name, _length) asm(#name "_length"); \
        return httpd_resp_send(req, CONCAT2(name, _start), CONCAT2(name, _length)); \
    }

esp_err_t get_resource(httpd_req_t *req)
{
    HTTP_CHECK("/",                           "text/html", index_html)
    HTTP_CHECK("/index.html",                 "text/html", index_html)

    HTTP_CHECK("/styles.css",                 "text/css", styles_css)

    HTTP_CHECK("/javascript.js",              "text/javascript", javascript_js)

    HTTP_CHECK("/browserconfig.xml",          "text/xml", browserconfig_xml)

    HTTP_CHECK("/site.webmanifest",           "application/json", site_webmanifest)
    HTTP_CHECK("/android-chrome-192x192.png", "image/png", android_chrome_192x192_png)
    HTTP_CHECK("/android-chrome-512x512.png", "image/png", android_chrome_512x512_png)
    HTTP_CHECK("/apple-touch-icon.png",       "image/png", apple_touch_icon_png)
    HTTP_CHECK("/favicon-16x16.png",          "image/png", favicon_16x16_png)
    HTTP_CHECK("/favicon-32x32.png",          "image/png", favicon_32x32_png)
    HTTP_CHECK("/mstile-70x70.png",           "image/png", mstile_70x70_png)
    HTTP_CHECK("/mstile-144x144.png",         "image/png", mstile_144x144_png)
    HTTP_CHECK("/mstile-150x150.png",         "image/png", mstile_150x150_png)
    HTTP_CHECK("/mstile-310x150.png",         "image/png", mstile_310x150_png)
    HTTP_CHECK("/mstile-310x310.png",         "image/png", mstile_310x310_png)


    HTTP_CHECK("/circle-stop-solid.svg",      "image/svg+xml", circle_stop_solid_svg)
    HTTP_CHECK("/circle-plus-solid.svg",      "image/svg+xml", circle_plus_solid_svg)
    HTTP_CHECK("/safari-pinned-tab.svg",      "image/svg+xml", safari_pinned_tab_svg)
    HTTP_CHECK("/favicon.svg",                "image/svg+xml", favicon_svg)

    HTTP_CHECK("/favicon.ico",                "image/vnd.microsoft.icon", favicon_ico)

    return httpd_resp_send_404(req);
}

esp_err_t get_temperature(httpd_req_t *req)
{
    const float temp = mcp9600_get_temp_F();
    char response[100];
    snprintf(response, sizeof(response), "%f", temp);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, response, strlen(response));
}
esp_err_t set_mode_handler(httpd_req_t *req)
{
    return ESP_OK;
}

static httpd_uri_t uri_list[] = {
    {
        .uri      = "/get_temp.json",
        .method   = HTTP_GET,
        .user_ctx = NULL,
        .handler = get_temperature
    },
    {
        .uri      = "/set_target_temp",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_target_temperature
    },
    {
        .uri      = "/set_downdraft_fan",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_downdraft_fan
    },
    {
        .uri      = "/set_convection_fan",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_convection_fan
    },
    {
        .uri      = "/set_light",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_light
    },
    {
        .uri      = "/set_cooling_fan",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_cooling_fan
    },
    {
        .uri      = "/set_use_top_element",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_use_top_element
    },
    {
        .uri      = "/set_use_bottom_element",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_use_bottom_element
    },
    {
        .uri      = "/set_stove_mode",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = set_stove_mode
    },
    {
        .uri      = "/get_state.json",
        .method   = HTTP_GET,
        .user_ctx = NULL,
        .handler = get_state
    },
    {
        .uri      = "/get_timers.json",
        .method   = HTTP_GET,
        .user_ctx = NULL,
        .handler = get_timers
    },
    {
        .uri      = "/add_timer",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = add_timer
    },
    {
        .uri      = "/rm_timer",
        .method   = HTTP_POST,
        .user_ctx = NULL,
        .handler = rm_timer
    },
    {
        .uri      = "/*",
        .method   = HTTP_GET,
        .user_ctx = NULL,
        .handler = get_resource
    },
};

static void start_webserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 20;

    printf("Starting webserver\n");

    const int err = httpd_start(&server, &config);
    if(err)
    {
        printf("Starting webserver FAILED!\n");
        return;
    }

    /* Register URI handlers */
    for(int i = 0; i < sizeof(uri_list)/sizeof(httpd_uri_t); i++)
        httpd_register_uri_handler(server, &uri_list[i]);
}

static esp_err_t stop_webserver(httpd_handle_t server)
{
    printf("Stopping webserver");
    return httpd_stop(server);
}

static void disconnect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (server)
    {
        if (stop_webserver(server) == ESP_OK)
        {
            server = NULL;
        }
        else
        {
            printf("Failed to stop http server");
        }
    }
}

static void connect_handler(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
    if (server == NULL)
    {
        start_webserver();
    }
}

int register_webserver()
{
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

    return ESP_OK;
}

esp_err_t get_content(httpd_req_t *req, char *content, size_t content_size)
{
    /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */

    if(req->content_len >= content_size)
    {
        printf("HTTP POST too large!\n");
        return ESP_FAIL;
    }
    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) /* 0 return value indicates connection closed */

    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) /* Check if timeout occurred */
        {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }

        printf("get content failed!!! %i\n", ret);
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }

    content[req->content_len] = 0;
    return ESP_OK;
}

esp_err_t ack_http_post(httpd_req_t *req)
{
    const char resp[] = "Data Retrieved";
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
}
