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

Audio g_audio;

size_t decode_mp3(std::vector<uint8_t> data) {
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

int main()
{
  stdio_init_all();
  cyw43_arch_init();
  // The pico will start when you start the terminal
  while (!stdio_usb_connected())
  {
  }

  g_audio.init_i2s();


  if (!Wifi::connect(WIFI_SSID, WIFI_PASS))
  {
    printf("Connect wifi error\n");
  }

  // Change this to the desired target
  char host[] = "marcai.blob.core.windows.net";
  char path[] = "/audio/mono/YourMom.mp3";
  // char host[] = "www.facebook.com";
  // char path[] = "/";

  Http_client http_client;
  Http_client::Http_request req;
  req.headers["user-agent"] = "curl/8.7.1";
  req.client = &http_client;
  req.hostname = host;
  req.path = path;
  req.callback_body = decode_mp3;
  http_client.request(&req);

  // req.path = "/audio?comp=list&prefix=mono";
  // http_client.request(&req);
  // http_client.http_get(host, path);
  while (true) {
  }
}
