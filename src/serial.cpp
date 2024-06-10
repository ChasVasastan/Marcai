#include <cstdio>
#include <string>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "serial.h"
#include "state.h"

void Serial::init() {
  printf("Serial communication is initialized...\n");
  uart_init(uart0, 115200);
  gpio_set_function(0, GPIO_FUNC_UART);
  gpio_set_function(1, GPIO_FUNC_UART);

  // Set 8 data bits, 1 stop bit, no parity
  uart_set_format(uart0, 8, 1, UART_PARITY_NONE);

  // Enable irq handler to be called on receive
  irq_set_exclusive_handler(UART0_IRQ, uart_on_rx);
  irq_set_enabled(UART0_IRQ, true);

  // Enable irq for rx only
  uart_set_irq_enables(uart0, true, false);
  printf("Initialised serial\n");
}

void Serial::uart_on_rx() {
  static std::string line;
  State &state = State::getInstance();
  while (uart_is_readable(uart0)) {
    // Read one char
    char c = uart_getc(uart0);
    if (c == '\n') {
      const char gesture_str[] = "gesture: ";
      const char prompt_str[] = "prompt: ";
      const char color_str[] = "color: ";
      if (auto npos = line.find(gesture_str); npos != std::string::npos) {
        std::string gesture = line.substr(npos+sizeof(gesture_str)-1, line.length());
        printf("gesture: %s\n", gesture.c_str());
        if (gesture == "None") {
        } else if (gesture == "Up") {
          state.play_song_flag = true;
        } else if (gesture == "Down") {
          state.stop_song_flag = true;
        } else if (gesture == "Left") {
          state.play_previous_song_flag = true;
        } else if (gesture == "Right") {
          state.play_next_song_flag = true;
        }
        line.clear();
      } else if (auto npos = line.find(color_str); npos != std::string::npos) {
        std::string color = line.substr(npos+sizeof(color_str)-1, line.length());
        printf("color: %s\n", color.c_str());
        if (color == "Red") {
          state.color_picked = Red;
        } else if (color == "Green") {
          state.color_picked = Green;
        } else if (color == "Blue") {
          state.color_picked = Blue;
        }
      } else if (auto npos = line.find(prompt_str); npos != std::string::npos) {
        std::string prompt = line.substr(npos+sizeof(prompt_str)-1, line.length());
        printf("Got prompt %s\n", prompt.c_str());
        line.clear();
      } else {
        printf("%s\n", line.c_str());
        line.clear();
      }
    } else {
      line.push_back(c);
    }
  }
}
