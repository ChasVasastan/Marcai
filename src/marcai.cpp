#include <stdio.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwipopts.h"
#include "lwip/altcp.h"
#include "lwip/dns.h"
#include "lwip/altcp_tls.h"

#include "wifi.h"
#include "http_client.h"
#include "audio.h"

#include <cJSON.h>

cJSON *json = cJSON_CreateObject();

Audio g_audio;

size_t decode_mp3(http::request *req, std::vector<uint8_t> data) {
  // Find next frame in buffer
  size_t offset = MP3FindSyncWord(data.data(), data.size());

  while (offset < data.size()) {
    int read;
    do {
      // Decode data while audio buffers are available
      read = g_audio.stream_decode(data.data() + offset,
                                   data.size() - offset);
    } while (read < 0);

    if (read == 0) {
      // Could not decode because of insufficient data
      return offset;
    }

    offset += read;
  }

  return offset;
}

size_t decode_xml(http::request *req, std::vector<uint8_t> data) {
  for (auto c : data) {
    putchar(c);
  }
  return data.size();
}

int main()
{
  stdio_init_all();
  cyw43_arch_init();
  g_audio.init_i2s();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected());

  if (!Wifi::connect(WIFI_SSID, WIFI_PASS))
  {
    printf("Connect wifi error\n");
  }

  // Change this to the desired target
  char host[] = "google-translate1.p.rapidapi.com";

  http::client http_client;
  http::request req[4];
  std::string body = "q=Detta%20%C3%A4r%20en%20svensk%20test%20text%20som%20ska%20avsl%C3%B6ja%20n%C3%A5got.";

  req[0].client = &http_client;
  req[0].hostname = host;
  req[0].headers["X-Rapidapi-Key"] = "insert-key-here";
  req[0].headers["Content-type"] = "application/x-www-form-urlencoded";
  req[0].headers["X-Rapidapi-Host"] = "google-translate1.p.rapidapi.com";
  req[0].path = "/language/translate/v2/detect";
  req[0].body = http::body_t(body.begin(), body.end());
  req[0].method = "POST";
  req[0].callback_body = decode_xml;

  http_client.request(&req[0]);
  while (true);
}
