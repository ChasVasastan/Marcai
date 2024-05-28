#include "image.h"

std::vector<uint16_t> Image::convert_rgb888_to_rgb565(std::vector<uint8_t> data) {
  std::vector<uint16_t> pixels;
  for (int i = 0; i < data.size(); i += 3) {
    uint16_t color = 0;
    color |= (data[i+0] >> 3) << 11;
    color |= (data[i+1] >> 2) << 5;
    color |= (data[i+2] >> 3);
    pixels.push_back(color);
  }
  return pixels;
}
