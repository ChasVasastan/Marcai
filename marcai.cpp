#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"

char ssid[] = "ssid";
char pass[] = "pass";

bool connect_wifi()
{
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect :(\n");

        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(250);

        return false;
    }

    printf("Connected :)\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return true;
}

int main()
{
    while (!stdio_usb_connected())
    {
    }

    stdio_init_all();
    cyw43_arch_init();

    if (connect_wifi())
    {
        printf("connect wifi error\n");
    }
}
