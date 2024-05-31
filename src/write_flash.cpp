#include "write_flash.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

#include <cstring>
#include <cstdio>


void Write_Flash::save_credentials(const std::string &ssid, const std::string password)
{
  char data[FLASH_PAGE_SIZE];
  std::memset(data, 0, FLASH_PAGE_SIZE);
  std::snprintf(data, FLASH_PAGE_SIZE, "SSID:%s;PASSWORD:%s", ssid.c_str(), password.c_str());

  uint32_t interrupts = save_and_disable_interrupts();

  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

  flash_range_program(FLASH_TARGET_OFFSET, reinterpret_cast<const uint8_t*>(data), FLASH_PAGE_SIZE);

  restore_interrupts(interrupts);
}

bool Write_Flash::load_credentials(std::string &ssid, std::string &password)
{
  
  char data[FLASH_PAGE_SIZE];
  const uint8_t *flash_target_contents = reinterpret_cast<const uint8_t*>(XIP_BASE + FLASH_TARGET_OFFSET);
  std::memcpy(data, flash_target_contents, FLASH_PAGE_SIZE);

  char ssid_buf[33] = {0};
  char password_buf[64] = {0};

  if (std::sscanf(data, "SSID:%32[^;];PASSWORD:%63[^;];", ssid_buf, password_buf) == 2)
  {
    ssid = ssid_buf;
    password = password_buf;
    printf("load_credentials received SSID: %s and Password: %s\n", ssid.c_str(), password.c_str());
    return !ssid.empty() && !password.empty();
  }
    return false;
}
