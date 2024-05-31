#include "cgi.h"
#include "pico/cyw43_arch.h"
#include "state.h"
#include "write_flash.h"

#include <string.h>
#include <stdlib.h>

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

    // Remove spaces in the SSID
    char new_ssid[sizeof(ssid)];
    size_t j = 0;
    for (size_t i = 0; i < sizeof(ssid) / sizeof(char); i++)
    {
        if (ssid[i] == '\0') {
            break;
        }
        if (ssid[i] == '+')
        {
            new_ssid[j++] = ' ';
            continue;
        }
        new_ssid[j++] = ssid[i];
    }
    new_ssid[j] = '\0';
    

    // Connect to the Wi-Fi network with the provided SSID and password
    printf("The data entered on the website: SSID: %s, Password: %s\n", new_ssid, password);

    printf("Saving Wi-Fi credentials to flash memory\n");
    write_flash.save_credentials(new_ssid, password);

    // Signal to switch the Wi-Fi mode
    State &state = State::getInstance();
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
