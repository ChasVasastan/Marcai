#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <string.h>
#include <stdlib.h>

#include "state.h"

// CGI handler to configure Wi-Fi
const char *cgi_configure_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    char ssid[32] = {0};
    char password[64] = {0};

    // Parse parameters
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "ssid") == 0) {
            strncpy(ssid, pcValue[i], sizeof(ssid) - 1);
        } else if (strcmp(pcParam[i], "password") == 0) {
            strncpy(password, pcValue[i], sizeof(password) - 1);
        }
    }

    // Connect to the Wi-Fi network with the provided SSID and password
    printf("The entered SSID: %s, Password: %s\n",ssid, password);

    State& state = State::getInstance();
    state.switch_cyw43_mode = true;
    
    return "/index.shtml";
}

// tCGI Struct
static const tCGI cgi_handlers[] = {
    { "/submit.cgi", cgi_configure_handler }
};

void cgi_init(void) {
    http_set_cgi_handlers(cgi_handlers, sizeof(cgi_handlers) / sizeof(tCGI));
}
