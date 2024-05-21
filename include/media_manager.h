/* -*- c++ -*- */
#ifndef MARCAI_MEDIA_MANAGER_H
#define MARCAI_MEDIA_MANAGER_H

#include "http_request.h"
#include "audio.h"

#include <string>

class media_manager
{
private:
  Audio audio;
  std::vector<http::url> playlist;
  uint8_t i = 0;
  bool playing = true;

public:
  http::url generate_url(std::string keywords);
  void get_playlist();
  void play();
  bool pause();
  void next();
  void previous();
  void init();
};

#endif /* MARCAI_MEDIA_MANAGER_H */
