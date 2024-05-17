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

public:

  http::url generate_url(std::string keywords);
  std::vector<http::url> get_playlist();
  void play(http::url url);
  void pause();
  void next();
  void previous();
  void init();
};

#endif /* MARCAI_MEDIA_MANAGER_H */
