#include "image.h"

std::vector<uint16_t> Image::convert_rgb888_to_rgb565(std::vector<uint8_t> data) {
  std::vector<uint16_t> pixels(data.size() / 3);
  for (int i = 0; i < pixels.size(); ++i) {
    uint16_t color = 0;
    color |= (data[i*3+0] >> 3) << 11;
    color |= (data[i*3+1] >> 2) << 5;
    color |= (data[i*3+2] >> 3);
    pixels[i] = color;
  }
  return pixels;
}
