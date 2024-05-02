#include "wifi.h"

#include "pico/cyw43_arch.h"

  // Function to connec to wifi
  bool Wifi::connect_wifi()
  {
    cyw43_arch_enable_sta_mode();

    printf("Connecting to %s\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
      printf("Failed to connect :(\n");

      return false;
    }

    printf("Connected to %s\n", WIFI_SSID);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return true;
  }
