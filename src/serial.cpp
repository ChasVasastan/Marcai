#include <cstdio>
#include <string>

#include "hardware/gpio.h"
#include "hardware/uart.h"

#include "serial.h"

void Serial::init() {
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
  while (uart_is_readable(uart0)) {
    // Read one char
    char c = uart_getc(uart0);
    if (c == '\n') {
      const char gesture_str[] = "gesture: ";
      const char prompt_str[] = "prompt: ";
      if (auto npos = line.find(gesture_str); npos != std::string::npos) {
        std::string gesture = line.substr(npos+sizeof(gesture_str)-1, line.length());
        printf("Got gesture %s\n", gesture.c_str());
        if (gesture == "None") {
        } else if (gesture == "Up") {
        } else if (gesture == "Down") {
        } else if (gesture == "Left") {
        } else if (gesture == "Right") {
        }
        line.clear();
      } else if (auto npos = line.find("prompt: "); npos != std::string::npos) {
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
