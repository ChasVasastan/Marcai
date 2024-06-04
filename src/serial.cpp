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
}

void Serial::uart_on_rx() {
  std::string line;
  while (uart_is_readable(uart0)) {
    // Read one char and echo it back
    char c = uart_getc(uart0);
    if (c == '\n') {
       if (auto npos = line.find("gesture: "); npos != std::string::npos) {
        std::string gesture = line.substr(npos, line.length());
        if (gesture == "None") {
        } else if (gesture == "Up") {
        } else if (gesture == "Down") {
        } else if (gesture == "Left") {
        } else if (gesture == "Right") {
        }
      }
    } else {
      line.push_back(c);
    }
    putchar(c);
  }
}
