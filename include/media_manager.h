/* -*- c++ -*- */
#ifndef MARCAI_MEDIA_MANAGER_H
#define MARCAI_MEDIA_MANAGER_H

#include "http_request.h"
#include "audio.h"
#include "screen.h"

#include <string>

class media_manager
{
private:
  Audio audio;
  Screen screen;
  std::vector<http::url> playlist;
  uint8_t playlist_index = 0;
  bool playing = true;
  http::request *req;

public:

  http::url generate_url(std::string keywords);
  void get_playlist();
  void play(http::url url);
  void play();
  void stop();
  void next();
  void previous();
  void init();
  void get_album_cover(http::url url);
  void currently_playing();
  bool is_playing();
};

#endif /* MARCAI_MEDIA_MANAGER_H */
