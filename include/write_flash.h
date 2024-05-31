#ifndef WRITE_FLASH_H
#define WRITE_FLASH_H

#include <string>
#include "hardware/flash.h"


struct wifi_credentials
{
  const char *ssid;
  const char *password;
};

class Write_Flash
{
public:
  Write_Flash();
  static constexpr uint32_t FLASH_TARGET_OFFSET = (1024 * 2048) - FLASH_SECTOR_SIZE;

  static void save_credentials(const std::string &ssid, const std::string password);
  static bool load_credentials(std::string &ssid, std::string &password);
};

extern Write_Flash write_flash;

#endif /* WRITE_FLASH_H */