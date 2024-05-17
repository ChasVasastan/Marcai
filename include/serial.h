/* -*- c++ -*- */
#ifndef MARCAI_SERIAL_H
#define MARCAI_SERIAL_H

class Serial {
public:
  /** @brief Initialise UART interface
   */
  static void init();

private:
  /** @brief ISR for UART receive
   */
  static void uart_on_rx();
};

#endif /* MARCAI_SERIAL_H */
