#include <stdio.h>

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
  // char host[] = "catfact.ninja";
  // char path[] = "/fact";

  Http_client http_client;
  http_client.http_get(host, path);

  http_client.resolve_dns(host);
  printf("Trying to resolve IP...\n");
  while (!http_client.is_ip_resolved())
  {
    //sleep_ms(500);
  }

  http_client.tls_tcp_setup(host);

  while (true)
  {
  }
}
