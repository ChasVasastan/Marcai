#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define WIFI_AP_SSID "Marcai"
#define WIFI_AP_PASSWORD "chas5678"

class Wifi_Config
{

public:
  static void setup_access_point();
  static void setting_netif();
  static void scan_networks();
};

#endif // WIFI_CONFIG_H
