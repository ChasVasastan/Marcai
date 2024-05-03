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


  if (!Wifi::connect_wifi())
  {
    printf("Connect wifi error\n");
  }

  // Change this to the desired target
  // char server_name[] = "storage.googleapis.com";
  // char uri[] = "/udio-artifacts-c33fe3ba-3ffe-471f-92c8-5dfef90b3ea3/samples/8ecb5ec94c774b04bdb3fd66d39c1b2b/1/Cosmic%20Canine%20Cruise.mp3";
  // char server_name[] = "catfact.ninja";
  // char uri[] = "/fact";

  char server_name[] = "marcai.blob.core.windows.net";
  char uri[] = "/audio/mono/YourMom.mp3";

  Http_client http_client;
  http_client.http_get(server_name, uri);

  http_client.resolve_dns(server_name);
  printf("Trying to resolve IP...\n");
  while (!http_client.is_ip_resolved())
  {
    //sleep_ms(500);
  }

  http_client.tls_tcp_setup(server_name);

  while (true)
  {
  }
}
