#include <cstdio>
#include <cstring>

#include "hardware/flash.h"
#include "hardware/sync.h"

#include "write_flash.h"

/**
 * @brief Saves wifi credentials to flash memory of the pico w.
 * 
 * This functions receives the SSID and password, formates them to a string,
 * removes memory to ensure it is empty and then allocates memory on the interal flash drive 
 * for the data
 * 
 * @param ssid The SSID for the chosen wifi network.
 * @param password The password for the chosen wifi network.
*/

void Write_Flash::save_credentials(const std::string &ssid, const std::string password)
{
  char data[FLASH_PAGE_SIZE];
  std::memset(data, 0, FLASH_PAGE_SIZE);
  std::snprintf(data, FLASH_PAGE_SIZE, "SSID:%s;PASSWORD:%s", ssid.c_str(), password.c_str());

  // Disable interrupts to ensure atomic operation (single indivisible instructions)
  uint32_t interrupts = save_and_disable_interrupts();

  // Erase the flash sector before writing
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);

  // Write the credentials to the flash memory
  flash_range_program(FLASH_TARGET_OFFSET, reinterpret_cast<const uint8_t*>(data), FLASH_PAGE_SIZE);

  // Restore interrupts
  restore_interrupts(interrupts);
}

/**
 * @brief Loads the wifi credentials from flash memory.
 * 
 * This function tries to retrieve the credentials saved in flash memory and parse them.
 * If they are successfully loaded, it returns true.
 * 
 * @param ssid Referes to a string where the loaded data will be stored.
 * @param password Referes to a string where the loaded data will be stored.
 * @return bool Returns true if the credentials are successfully loaded.
*/

bool Write_Flash::load_credentials(std::string &ssid, std::string &password)
{
  char data[FLASH_PAGE_SIZE];
  const uint8_t *flash_target_contents = reinterpret_cast<const uint8_t*>(XIP_BASE + FLASH_TARGET_OFFSET);
  std::memcpy(data, flash_target_contents, FLASH_PAGE_SIZE);

  char ssid_buf[33] = {0};
  char password_buf[64] = {0};

  // Parse the stored credentials
  if (std::sscanf(data, "SSID:%32[^;];PASSWORD:%63[^;];", ssid_buf, password_buf) == 2)
  {
    ssid = ssid_buf;
    password = password_buf;
    printf("load_credentials received SSID: %s and Password: %s\n", ssid.c_str(), password.c_str());
    return !ssid.empty() && !password.empty();
  }
    return false;
}
