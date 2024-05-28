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

#include "screen.h"
#include "spng.h"
#include "ezxml.h"
#include <cstdint>
#include <cstdio>
#include <vector>
#include "audio.h"
#include "http_client.h"
#include <malloc.h>
#include <cstdlib>
#include <cstring>
#include "hardware/flash.h"
#include "image.h"

extern Screen sc;

uint32_t getTotalHeap(void) {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
  struct mallinfo m = mallinfo();
  return getTotalHeap() - m.uordblks;
}

static size_t decode_mp3(http::request *req, std::vector<uint8_t> data)
{
  auto audio = reinterpret_cast<Audio *>(req->arg);
  // Find next frame in buffer
  size_t offset = MP3FindSyncWord(data.data(), data.size());

  while (offset < data.size()) {
    int read;
    do {
      // Decode data while audio buffers are available
      read = audio->stream_decode(data.data() + offset, data.size() - offset);
    } while (read < 0);

    if (read == 0) {
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

/** @brief Write http body to flash memory
 */
static uint8_t __in_flash() __aligned(FLASH_SECTOR_SIZE) album_cover[FLASH_SECTOR_SIZE * 30];
static size_t decode_png(http::request *req, std::vector<uint8_t> data)
{
  auto *addr = reinterpret_cast<uint32_t*>(req->arg);
  uint32_t size = FLASH_PAGE_SIZE * (data.size() / FLASH_PAGE_SIZE);
  if (req->state == http::state::DONE)
    size += FLASH_PAGE_SIZE;
  data.reserve(size);
  uint32_t ints = save_and_disable_interrupts();
  const uint8_t *buffer = data.data();
  flash_range_program(*addr, buffer, size);
  *addr += size;
  restore_interrupts(ints);
  return size;
}

/** @brief Copy png data to buffer
 */
static int decode_png_stream(spng_ctx *ctx, void *arg, void *dst, size_t size) {
  auto offset = reinterpret_cast<uint32_t*>(arg);
  memcpy(dst, album_cover + *offset, size);
  *offset += size;
  return 0;
}

/** @brief Callback for when http request is done to decode and
 *  display image on screen.
 */
static void png_result(http::request *req) {
  auto body = reinterpret_cast<std::vector<uint8_t>*>(req->arg);
  int error;
  spng_ctx *ctx;
  uint32_t offset = 0;
  spng_ihdr ihdr;
  ctx = spng_ctx_new(0);
  spng_set_png_stream(ctx, decode_png_stream, &offset);
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
      sc.display(rgb565.data(), rgb565.size());
    else
      sc.display_row(i, rgb565.data(), rgb565.size());
  }
  spng_ctx_free(ctx);
}

void media_manager::play(http::url url)
{
  std::string host, path;
  if (auto npos = url.find("https://"); npos != std::string::npos) {
    url = url.substr(npos+8, url.length());
  }

  if (auto npos = url.find("/"); npos != std::string::npos) {
    host = url.substr(0, npos);
    path = url.substr(npos, url.length());
  }

  // https://marcai.blob.core.windows.net/audio/mono/YourMom.mp3
  http::client http_client;
  http::request req;
  req.client = &http_client;
  req.hostname = host.c_str();
  req.path = path.c_str();
  req.method = "GET";
  req.callback_body = decode_mp3;
  req.arg = &audio;

  http_client.request(&req);
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
    //printf("waiting ");
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

void media_manager::stop()
{
  req->abort_request();
  delete req;
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

void media_manager::get_album_cover(http::url url) {
  http::client http_client;
  http::request req;
  // Memory address for flash data
  uint32_t addr = reinterpret_cast<uint32_t>(album_cover) - XIP_BASE;
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(addr, sizeof(album_cover));
  restore_interrupts(ints);

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
  req.callback_body = decode_png;
  req.callback_result = png_result;
  req.arg = &addr;

  http_client.request(&req);
  while (req.state != http::state::DONE && req.state != http::state::FAILED);
}
