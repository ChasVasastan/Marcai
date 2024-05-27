#include "http_server.h"
#include "lwip/apps/httpd.h"
#include "wifi_config.h"

#include <string.h>
#include <stdio.h>

Wifi_Config wifi_config;

const char *config_page_html =
    "<html>"
    "<body>"
    "<h1>Configure Wi-Fi</h1>"
    "<form action=\"/configure\" method=\"POST\">"
    "SSID: <input type=\"text\" name=\"ssid\"><br>"
    "Password: <input type=\"password\" name=\"password\"><br>"
    "<input type=\"submit\" value=\"Connect\">"
    "</form>"
    "</body>"
    "</html>";

static const char *response_headers =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n";

static u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
    // Handle SSI here
    return 0;
}

static const char *ssi_tags[] = {
    "tag1",
    "tag2"
};

const char *cgi_handler(int index, int num_params, char *params[], char *values[]) {
    char ssid[32] = {0};
    char password[64] = {0};
    
    for (int i = 0; i < num_params; i++) {
        if (strcmp(params[i], "ssid") == 0) {
            strncpy(ssid, values[i], sizeof(ssid) - 1);
        } else if (strcmp(params[i], "password") == 0) {
            strncpy(password, values[i], sizeof(password) - 1);
        }
    }

    printf("Received SSID: %s, Password: %s\n", ssid, password);
    wifi_config.connect_to_network(ssid, password);

    const char *confirm_page =
        "<html>"
        "<body>"
        "<h1>Connecting to network...</h1>"
        "</body>"
        "</html>";

/*
    struct fs_file file;
    file.data = (const char *)confirm_page;
    file.len = strlen(confirm_page);

    httpd_resp_send(&file);
*/

    return "/done";
}
static const tCGI cgi_uri_handlers[] = {
    {"/configure", cgi_handler},
};


void Http_Server::init() {
    printf("Web page is trying to initialize\n");

    httpd_init();
    http_set_ssi_handler(ssi_handler, ssi_tags, sizeof(ssi_tags)/sizeof(char*));
    http_set_cgi_handlers(cgi_uri_handlers, sizeof(cgi_uri_handlers) / sizeof(tCGI));

    printf("Web page is initialized\n");
}
