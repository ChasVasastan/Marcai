#include <stdio.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "media_manager.h"
#include "wifi.h"
//#include "serial.h"

#include <cJSON.h>

enum
{
  GESTURE_NONE = -1,
  GESTURE_UP = 0,
  GESTURE_DOWN = 1,
  GESTURE_LEFT = 2,
  GESTURE_RIGHT = 3
};

cJSON *json = cJSON_CreateObject();

media_manager manager;

int main()
{
  stdio_init_all();
  cyw43_arch_init();
  //Serial::init();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected())
    ;
  manager.init();

  if (!Wifi::connect(WIFI_SSID, WIFI_PASS))
  {
    printf("Connect wifi error\n");
  }

  std::vector<http::url> playlist = manager.get_playlist();
  manager.play(playlist[0]);
  while (true)
  {
  /*
    switch (gesture)
    {
    case GESTURE_UP:
      manager.play();
      break;
    case GESTURE_DOWN:
      manager.pause();
      break;
    case GESTURE_LEFT:
      manager.previous();
      break;
    case GESTURE_RIGHT:
      manager.next();
      break;
    default:
      break;
    }
  */
  }

}
