// -*- c++ -*-
#ifndef MARCAI_IMAGE_H
#define MARCAI_IMAGE_H

#include <cstdint>
#include <vector>

class Image {
public:
  /** @brief Convert pixel data from rgb888 to rgb565 format
   *
   *  @param data pixel data input
   *  @return pixel data output
   */
  static std::vector<uint16_t> convert_rgb888_to_rgb565(std::vector<uint8_t> data);
};

#endif /* MARCAI_IMAGE_H */
