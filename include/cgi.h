#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include <string.h>
#include <stdlib.h>

// Function to connect to Wi-Fi
void connect_to_wifi(const char *ssid, const char *password) {
    cyw43_arch_disable_ap_mode();
    cyw43_arch_enable_sta_mode();
    printf("Connecting to %s\n", ssid);
  if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("Failed to connect, trying again...\n");
  }
}

// CGI handler to configure Wi-Fi
const char *cgi_configure_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    char ssid[32] = {0};
    char password[32] = {0};

    // Parse parameters
    for (int i = 0; i < iNumParams; i++) {
        if (strcmp(pcParam[i], "ssid") == 0) {
            strncpy(ssid, pcValue[i], sizeof(ssid) - 1);
        } else if (strcmp(pcParam[i], "password") == 0) {
            strncpy(password, pcValue[i], sizeof(password) - 1);
        }
    }

    // Connect to the Wi-Fi network with the provided SSID and password
    connect_to_wifi(ssid, password);

    // Send a confirmation page back to the user
    return "/index.shtml";
}

// tCGI Struct
static const tCGI cgi_handlers[] = {
    { "/submit.cgi", cgi_configure_handler }
};

void cgi_init(void) {
    http_set_cgi_handlers(cgi_handlers, sizeof(cgi_handlers) / sizeof(tCGI));
}
