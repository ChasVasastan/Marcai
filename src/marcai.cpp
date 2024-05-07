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

size_t decode_xml(std::vector<uint8_t> data) {
  for (auto c : data) {
    putchar(c);
  }
  return data.size();;
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
  Http_client::Http_request req[2];
  req[0].headers["user-agent"] = "curl/8.7.1";
  req[0].client = &http_client;
  req[0].hostname = host;
  req[0].path = path;
  req[0].callback_body = decode_mp3;
  http_client.request(&req[0]);

  req[1].headers["user-agent"] = "curl/8.7.1";
  req[1].client = &http_client;
  req[1].hostname = host;
  req[1].path = "/audio?comp=list&prefix=mono";
  req[1].callback_body = decode_xml;
  http_client.request(&req[1]);

  while (true) {
  }
}
