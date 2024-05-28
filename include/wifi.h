#ifndef WIFI_H
#define WIFI_H

#include <string>

class Wifi
{
private:
public:
  Wifi()
  {
  }

  static bool connect(std::string ssid, std::string pass);
};

#endif /* WIFI_H */
