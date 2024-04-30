#ifndef WIFI_H
#define WIFI _H

#include <cstring>

class Wifi
{
private:
  // char ssid[] = "Tele2_af3a52";
  // char pass[] = "vwykzm4c";
  char ssid[32];
  char pass[32];

public:
  Wifi(const char *wifi_name, const char *wifi_pass)
  {
    strncpy(this->ssid, wifi_name, sizeof(this->ssid - 1));
    ssid[sizeof(ssid) - 1] = '\0';
    strncpy(this->pass, wifi_pass, sizeof(this->pass - 1));
    ssid[sizeof(ssid) - 1] = '\0';
  }

  static bool connect_wifi(const char *wifi_name, const char *wifi_pass);
};

#endif /* WIFI_H */
