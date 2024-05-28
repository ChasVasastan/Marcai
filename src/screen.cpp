#include "screen.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include <cstdint>
#include "st7789_lcd.pio.h"

#include <stdio.h>

void Screen::init() {
  printf("Screen init start\n");
  uint offset = pio_add_program(pio, &st7789_lcd_program);
  printf("add program\n");
  st7789_lcd_program_init(pio, sm, offset, PIN_DIN, PIN_CLK, 2.0f);
  printf("program init\n");

  gpio_init(PIN_CS);
  gpio_init(PIN_DC);
  gpio_init(PIN_RESET);
  gpio_init(PIN_BL);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  gpio_set_dir(PIN_RESET, GPIO_OUT);
  gpio_set_dir(PIN_BL, GPIO_OUT);

  gpio_put(PIN_CS, 1);
  gpio_put(PIN_RESET, 1);
  lcd_init(pio, sm, st7789_init_seq);
  gpio_put(PIN_BL, 1);
  printf("Screen init success\n");
}

void Screen::clear(uint16_t colour) {
  st7789_start_pixels(pio, sm);
  for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    st7789_lcd_put(pio, sm, colour & 0xff);
    st7789_lcd_put(pio, sm, colour >> 8);
  }
}

void Screen::display_row(uint16_t row, uint16_t* data, size_t size) {
  for (int i = 0; i < size; i++) {
    st7789_lcd_put(pio, sm, data[i] >> 8);
    st7789_lcd_put(pio, sm, data[i] & 0xff);
  }
}

void Screen::display(uint16_t* data, size_t size) {
  st7789_start_pixels(pio, sm);
  for (int i = 0; i < size; i++) {
    st7789_lcd_put(pio, sm, data[i] >> 8);
    st7789_lcd_put(pio, sm, data[i] & 0xff);
  }
}

void Screen::vertical_scroll(uint16_t offset) {
  const uint8_t cmd[3] = {0x37, static_cast<uint8_t>(offset >> 8),
                          static_cast<uint8_t>(offset & 0xff)};
  lcd_write_cmd(pio, sm, cmd, 3);
}

void Screen::lcd_set_dc_cs(bool dc, bool cs) {
  sleep_us(1);
  gpio_put_masked((1u << PIN_DC) | (1u << PIN_CS), !!dc << PIN_DC | !!cs << PIN_CS);
  sleep_us(1);
}

void Screen::lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count) {
  st7789_lcd_wait_idle(pio, sm);
  lcd_set_dc_cs(0, 0);
  st7789_lcd_put(pio, sm, *cmd++);
  if (count >= 2) {
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 0);
    for (size_t i = 0; i < count - 1; ++i)
      st7789_lcd_put(pio, sm, *cmd++);
  }
  st7789_lcd_wait_idle(pio, sm);
  lcd_set_dc_cs(1, 1);
}

void Screen::lcd_init(PIO pio, uint sm, const uint8_t *init_seq) {
  const uint8_t *cmd = init_seq;
  while (*cmd) {
    printf("Write cmd %2x %3d", *cmd, cmd[1] * 5);
    for (int i = 0; i < *cmd; ++i)
      printf(" %02x", cmd[i+2]);
    putchar('\n');
    lcd_write_cmd(pio, sm, cmd + 2, *cmd);
    sleep_ms(*(cmd + 1) * 5);
    cmd += *cmd + 2;
  }
}

void Screen::st7789_start_pixels(PIO pio, uint sm) {
  uint8_t cmd = 0x2c; // RAMWR
  lcd_write_cmd(pio, sm, &cmd, 1);
  lcd_set_dc_cs(1, 0);
}
