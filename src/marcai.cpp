#include <stdio.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "media_manager.h"
#include "wifi.h"
// #include "serial.h"

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
  // Serial::init();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected())
    ;
  manager.init();

  if (!Wifi::connect(WIFI_SSID, WIFI_PASS))
  {
    printf("Connect wifi error\n");
  }

  manager.get_playlist();

  const uint PIN_BUTTON1 = 2;
  const uint PIN_BUTTON2 = 3;
  const uint PIN_BUTTON3 = 4;
  const uint PIN_BUTTON4 = 5;

  gpio_init(PIN_BUTTON1);
  gpio_set_dir(PIN_BUTTON1, GPIO_IN);
  gpio_pull_up(PIN_BUTTON1);

  gpio_init(PIN_BUTTON2);
  gpio_set_dir(PIN_BUTTON2, GPIO_IN);
  gpio_pull_up(PIN_BUTTON2);

  gpio_init(PIN_BUTTON3);
  gpio_set_dir(PIN_BUTTON3, GPIO_IN);
  gpio_pull_up(PIN_BUTTON3);

  gpio_init(PIN_BUTTON4);
  gpio_set_dir(PIN_BUTTON4, GPIO_IN);
  gpio_pull_up(PIN_BUTTON4);

  while (true)
  {
    bool button1_pressed = !gpio_get(PIN_BUTTON1);
    bool button2_pressed = !gpio_get(PIN_BUTTON2);
    bool button3_pressed = !gpio_get(PIN_BUTTON3);
    bool button4_pressed = !gpio_get(PIN_BUTTON4);

    const uint DEBOUNCE_TIME_MS = 200;
    static uint64_t last_press_time = 0;
    uint64_t current_time = to_ms_since_boot(get_absolute_time());

    if (current_time - last_press_time > DEBOUNCE_TIME_MS)
    {

      printf("%d", button1_pressed);
      printf("%d", button2_pressed);
      printf("%d", button3_pressed);
      printf("%d", button4_pressed);

      if (button1_pressed)
      {
        printf("Play\n");
        manager.play();
      }
      else if (button2_pressed)
      {
        if (!manager.pause())
        {
          printf("Paused\n");
        } else if (manager.pause())
        {
          printf("Unpaused\n");
        }
        
      }
      else if (button3_pressed)
      {
        printf("Playing next song\n");
        manager.next();
      }
      else if (button4_pressed)
      {
        printf("Playing previous song\n");
        manager.previous();
      }
      last_press_time = current_time;
    }

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
