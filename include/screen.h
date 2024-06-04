// -*- c++ -*-
#ifndef MARCAI_SCREEN_H
#define MARCAI_SCREEN_H

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

#define PIN_DIN   19
#define PIN_CLK   18
#define PIN_CS    17
#define PIN_DC    10
#define PIN_RESET 11
#define PIN_BL    22

#include <cstdint>
#include "hardware/pio.h"

class Screen {
private:
  PIO pio = pio1;
  uint sm = 3;

  // Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
  // Note the delays have been shortened a little
  static constexpr uint8_t st7789_init_seq[] = {
    1, 20, 0x01,         // Software reset
    1, 10, 0x11,         // Exit sleep mode
    2, 2, 0x3a, 0x55,    // Set colour mode to 16 bit
    2, 0, 0x36, 0x00,    // Set MADCTL: row then column, refresh is bottom to top ????
    5, 0, 0x2a, 0x00, 0x00, SCREEN_WIDTH >> 8, SCREEN_WIDTH & 0xff,   // CASET: column addresses
    5, 0, 0x2b, 0x00, 0x00, SCREEN_HEIGHT >> 8, SCREEN_HEIGHT & 0xff, // RASET: row addresses
    1, 2, 0x21,          // Inversion on, then 10 ms delay (supposedly a hack?)
    1, 2, 0x13,          // Normal display on, then 10 ms delay
    1, 2, 0x29,          // Main screen turn on, then wait 500 ms
    7, 2, 0x33, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x50, // Vertical scrolling setup
    0                    // Terminate list
  };
  static inline void lcd_set_dc_cs(bool dc, bool cs);
  static inline void lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count);
  static inline void lcd_init(PIO pio, uint sm, const uint8_t *init_seq);
  static inline void st7789_start_pixels(PIO pio, uint sm);

public:

  /** @brief Initialise screen
   */
  void init();

  /** @brief Start command for writing pixels
   */
  void start_pixels();

  /** @brief Write pixels to screen
   *
   * @param data source data to write
   * @param size data size
   */
  void display(uint16_t *data, size_t size);

  /** @brief Clear screen to colour
   *
   * @note Must be initiated by start_pixels() command
   *
   * @param colour
   */
  void clear(uint16_t colour);
  void vertical_scroll(uint16_t offset);
};

#endif /* MARCAI_SCREEN_H */
