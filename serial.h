/* -*- c++ -*- */
#ifndef MARCAI_SERIAL_H
#define MARCAI_SERIAL_H

#include "hardware/uart.h"
#include "hardware/gpio.h"

#include <cstdio>

class Serial {
public:
  static void init() {
    uart_init(uart0, 9600);
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

private:
  static void uart_on_rx() {
    while (uart_is_readable(uart0)) {
      char c = uart_getc(uart0);
      putchar(c);
      uart_putc(uart0, c);
    }
  }
};

#endif /* MARCAI_SERIAL_H */
