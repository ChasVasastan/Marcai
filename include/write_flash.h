#ifndef WRITE_FLASH_H
#define WRITE_FLASH_H

#include <string>

struct wifi_credentials
{
  const char *ssid;
  const char *password;
};

class Write_Flash
{
private:
  static const uint32_t FLASH_TARGET_OFFSET = 256 * 1024;
  static const uint32_t FLASH_PAGE_SIZE = 256;
  static const uint32_t FLASH_SECTOR_SIZE = 4096;
  const uint8_t *flash_target_contents;

public:
  Write_Flash();
  void save_credentials(const std::string &ssid, const std::string password);
  bool load_credentials(std::string &ssid, std::string &password);
};

extern Write_Flash write_flash;

#endif /* WRITE_FLASH_H */