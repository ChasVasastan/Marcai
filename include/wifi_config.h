#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#define WIFI_AP_SSID "Marcai"
#define WIFI_AP_PASSWORD "chas5678"

class Wifi_Config
{

public:
  void setup_access_point();
  void setting_netif();
  void connect_to_network(const char *ssid, const char *password);
  void scan_networks();
};

#endif // WIFI_CONFIG_H