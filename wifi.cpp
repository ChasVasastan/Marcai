#include "wifi.h"

#include "pico/cyw43_arch.h"

  // Function to connec to wifi
  bool Wifi::connect_wifi(const char* wifi_name,const char* wifi_pass)
  {
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(wifi_name, wifi_pass, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
      printf("Failed to connect :(\n");

      return false;
    }

    printf("Connected to %s\n", wifi_name);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    return true;
  }
