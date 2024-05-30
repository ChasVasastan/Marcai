#include "wifi.h"

#include "pico/cyw43_arch.h"

  // Function to connec to wifi
bool Wifi::connect(std::string ssid, std::string pass)
{
  cyw43_arch_enable_sta_mode();

  printf("Connecting to %s\n...", ssid.c_str());
  if (cyw43_arch_wifi_connect_timeout_ms(ssid.c_str(), pass.c_str(), CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    return false;
  }

  printf("Connected to %s\n", ssid.c_str());
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
  return true;
}
