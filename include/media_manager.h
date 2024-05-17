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

public:
  media_manger(){};

  http::url generate_url(std::string keywords);
  void get_playlist();
  void play(http::url url);
  void pause();
  void next();
  void previous();
  void init();
};

#endif /* MARCAI_MEDIA_MANAGER_H */
