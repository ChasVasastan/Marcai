#include "media_manager.h"

#include <vector>

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"
#include "pico/audio_i2s.h"

#include "ezxml.h"
#include "audio.h"
#include "wifi.h"
#include "http_client.h"

static size_t decode_mp3(http::request *req, std::vector<uint8_t> data)
{
  // printf("Got body %ld\n", data.size());
  auto audio = reinterpret_cast<Audio *>(req->arg);
  // Find next frame in buffer
  size_t offset = MP3FindSyncWord(data.data(), data.size());

  while (offset < data.size())
  {
    int read;
    do
    {
      // Decode data while audio buffers are available
      read = audio->stream_decode(data.data() + offset, data.size() - offset);
      // printf("Trying to decode data: (%d) %ld\n", read, offset);
    } while (read < 0);

    if (read == 0)
    {
      // Could not decode because of insufficient data
      return offset;
    }

    offset += read;
  }

  return offset;
}

static size_t decode_playlist(http::request *req, std::vector<uint8_t> data)
{
  auto body = reinterpret_cast<std::vector<char> *>(req->arg);
  body->insert(body->end(), data.begin(), data.end());
  return data.size();
}

http::url media_manager::generate_url(std::string keywords)
{
  return http::url();
}

void media_manager::get_playlist()
{

  // Change this to the desired target
  char host[] = "marcai.blob.core.windows.net";
  std::vector<char> body;
  if (req == nullptr)
  {
    req = new http::request;
  }

  req->client = &http_client;
  req->hostname = host;
  req->path = "/audio?comp=list&prefix=mono";
  req->method = "GET";
  req->callback_body = decode_playlist;
  req->arg = &body;

  http_client.request(req);
  while (req->state != http::state::DONE && req->state != http::state::FAILED)
  {
    printf("waiting ");
    asm("nop");
  }

  ezxml_t xml = ezxml_parse_str(body.data(), body.size());
  ezxml_t blobs = ezxml_child(xml, "Blobs");
  for (ezxml_t blob = ezxml_child(blobs, "Blob"); blob; blob = blob->next)
  {
    playlist.emplace_back(ezxml_child(blob, "Url")->txt);
  }

  for (auto i : playlist)
  {
    printf("%s\n", i.c_str());
  }

  ezxml_free(xml);
  delete req;
  req = nullptr;

  // https://marcai.blob.core.windows.net/audio?comp=list&prefix=mono
}

void media_manager::play()
{
  if (playlist_index >= playlist.size())
  {
    playlist_index = 0;
  }

  http::url url = playlist[playlist_index];

  std::string host, path;
  printf("URL before: %s\n", url.c_str());
  if (auto npos = url.find("https://"); npos != std::string::npos)
  {
    url = url.substr(npos + 8, url.length());
  }
  printf("URL after: %s\n", url.c_str());

  if (auto npos = url.find("/"); npos != std::string::npos)
  {
    host = url.substr(0, npos);
    path = url.substr(npos, url.length());
  }
  printf("Playing %s\n", path.c_str());

  req = new http::request;
  if (req == nullptr)
  {
    panic("Out of memory in the play function\n");
  }

  req->client = &http_client;
  req->hostname = host.c_str();
  req->path = path.c_str();
  req->method = "GET";
  req->callback_body = decode_mp3;
  req->arg = &audio;

  http_client.request(req);
  printf("HTTP request made\n");
}

bool media_manager::pause_play()
{
  printf("Playing %s\n", playing ? "true" : "false");
  if (playing)
  {
    audio.pause();
  } else {
    audio.play();
  }

  playing = !playing;
  return playing;
}

void media_manager::next()
{
  playlist_index++;
  if (req != nullptr)
  {
    req->abort_request();
    delete req;
  }
  play();
}

void media_manager::previous()
{
  playlist_index--;
  if (req != nullptr)
  {
    req->abort_request();
    delete req;
  }
  play();
}

void media_manager::init()
{
  audio.init_i2s();
}
