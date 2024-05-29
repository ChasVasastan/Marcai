#include "audio.h"
#include "ezxml.h"
#include "hardware/flash.h"
#include "http_client.h"
#include "http_request.h"
#include "image.h"
#include "media_manager.h"
#include "screen.h"
#include "spng.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <malloc.h>
#include <vector>

extern Screen screen;

uint32_t getTotalHeap(void) {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
  struct mallinfo m = mallinfo();
  return getTotalHeap() - m.uordblks;
}

void media_manager::play(http::url url)
{
  if (req) {
    req->abort_request();
    delete req;
  }

  if (auto npos = url.find("/audio/mono"); npos != std::string::npos) {
    http::url cover = url;
    cover.replace(npos, 11, "/image/cover");
    cover = cover.replace(cover.size() - 3, 3, "png");
    printf("Cover image: %s\n", cover.c_str());
    get_album_cover(cover);
  }

  std::string host, path;
  if (auto npos = url.find("https://"); npos != std::string::npos) {
    url = url.substr(npos+8, url.length());
  }

  if (auto npos = url.find("/"); npos != std::string::npos) {
    host = url.substr(0, npos);
    path = url.substr(npos, url.length());
  }

  req = new http::request;

  req->client = &http_client;
  req->hostname = host.c_str();
  req->path = path.c_str();
  req->method = "GET";

  http_client.request(req);
  playing = true;
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
  auto playlist_body = [&](http::request *req, std::vector<uint8_t> data) {
    body.insert(body.end(), data.begin(), data.end());
    return data.size();
  };

  auto decode_playlist = [&](http::request *req) {
    ezxml_t xml = ezxml_parse_str(body.data(), body.size());
    ezxml_t blobs = ezxml_child(xml, "Blobs");
    for (ezxml_t blob = ezxml_child(blobs, "Blob"); blob; blob = blob->next) {
      playlist.emplace_back(ezxml_child(blob, "Url")->txt);
      printf("%s\n", playlist[playlist.size() - 1].c_str());
    }

    ezxml_free(xml);
  };

  http::request req;
  req.client = &http_client;
  req.hostname = host;
  req.path = "/audio?comp=list&prefix=mono";
  req.method = "GET";
  req.callback_body = playlist_body;
  req.arg = &body;
  req.callback_result = decode_playlist;

  http_client.request(&req);

  // wait blocking for request to finish
  while (req.state != http::state::DONE && req.state != http::state::FAILED);
}

void media_manager::play()
{
  if (playlist.empty())
    return;
  if (playlist_index >= playlist.size())
  {
    playlist_index = 0;
  }

  http::url url = playlist[playlist_index];
  play(url);
}

void media_manager::stop()
{
  if (req) {
    req->abort_request();
    delete req;
  }
  req = nullptr;
  playing = false;
}

void media_manager::next()
{
  playlist_index++;
  if (req) {
    req->abort_request();
    delete req;
  }
  req = nullptr;
  play();
}

void media_manager::previous()
{
  playlist_index--;
  if (req) {
    req->abort_request();
    delete req;
  }
  req = nullptr;
  play();
}

void media_manager::init()
{
  audio.init_i2s();
}

void media_manager::get_album_cover(http::url url) {
  http::request req;
  // Memory address for flash data
  if (auto npos = url.find("https://"); npos != std::string::npos) {
    url = url.substr(npos+8, url.length());
  }

  std::string host, path;
  if (auto npos = url.find("/"); npos != std::string::npos) {
    host = url.substr(0, npos);
    path = url.substr(npos, url.length());
  }

  req.client = &http_client;
  req.hostname = host.c_str();
  req.path = path.c_str();
  req.method = "GET";

  http_client.request(&req);

  int error;
  spng_ctx *ctx;
  uint32_t offset = 0;
  spng_ihdr ihdr;
  auto decode_png_stream = [](spng_ctx *ctx, void *arg, void *dst, size_t size) {
    auto req = reinterpret_cast<http::request*>(arg);
    int offset = 0;

    // if (req->status != 200)
    //   return static_cast<int>(SPNG_IO_ERROR);
    do {
      printf("PNG data %d (%ld)", offset, size);
      int len = req->peek(reinterpret_cast<uint8_t*>(dst) + offset,
                          size - offset);
      offset += len;
      printf(" read %d\n", len);
    } while (offset < size);
    req->skip(size);
    return static_cast<int>(SPNG_OK);
  };

  ctx = spng_ctx_new(0);
  spng_set_png_stream(ctx, decode_png_stream, &req);
  spng_decode_image(ctx, NULL, 0, SPNG_FMT_RGB8, SPNG_DECODE_PROGRESSIVE);
  spng_get_ihdr(ctx, &ihdr);
  printf("PNG image (%d,%d)\n", ihdr.width, ihdr.height);
  for (int i = 0; i < ihdr.height; ++i) {
    std::vector<uint8_t> rgb888(240 * 3);
    error = spng_decode_row(ctx, rgb888.data(), rgb888.size());
    std::vector<uint16_t> rgb565 = Image::convert_rgb888_to_rgb565(rgb888);
    // Display image data on screen, first row initiate screen write
    // and subsequent rows just write data
    if (i == 0)
      screen.display(rgb565.data(), rgb565.size());
    else
      screen.display_row(i, rgb565.data(), rgb565.size());
  }
  spng_ctx_free(ctx);
}

bool media_manager::is_playing() {
   return playing;
}

void media_manager::continue_playing() {
  if (req) {
    uint8_t buffer[1024];
    size_t size = req->peek(buffer, sizeof(buffer));
    int offset = MP3FindSyncWord(buffer, size);

    if (offset < 0)
      return;

    while (offset < size) {
      int read = audio.stream_decode(buffer + offset, size - offset);
      if (read <= 0)
        break;
      offset += read;
    }

    req->skip(offset);
  }
}
