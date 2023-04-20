
idf_component_register(
    SRCS
        "main.c"
        "i2c.c" "i2c.h"
        "led.c" "led.h"
        "httpd.c" "httpd.h"
        "urldecode.c" "urldecode.h"
        "wifi_sta.c"
        "mcp9600.c" "mcp9600.h"
        "stovectrl.cpp" "stovectrl.h"
        "cooktimers.cpp" "cooktimers.h"

    EMBED_TXTFILES
        "web/styles.css"
        "web/index.html"
        "web/javascript.js"
        "web/browserconfig.xml"
        "web/site.webmanifest"
        "web/android-chrome-192x192.png"
        "web/android-chrome-512x512.png"
        "web/apple-touch-icon.png"
        "web/favicon-16x16.png"
        "web/favicon-32x32.png"
        "web/mstile-70x70.png"
        "web/mstile-144x144.png"
        "web/mstile-150x150.png"
        "web/mstile-310x150.png"
        "web/mstile-310x310.png"
        "web/favicon.svg"
        "web/safari-pinned-tab.svg"
        "web/favicon.ico"
        "web/circle-stop-solid.svg"
        "web/circle-plus-solid.svg"


    INCLUDE_DIRS
        "."
)
