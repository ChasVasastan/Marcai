#include <stdio.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/timeouts.h"
#include "pico/multicore.h"
#include "lwip/apps/httpd.h"
#include "lwipopts.h"

#include "media_manager.h"
#include "wifi.h"
#include "state.h"
// #include "serial.h"
#include "wifi_config.h"
#include "cgi.h"
#include "write_flash.h"

enum
{
  GESTURE_NONE = -1,
  GESTURE_UP = 0,
  GESTURE_DOWN = 1,
  GESTURE_LEFT = 2,
  GESTURE_RIGHT = 3
};

const uint PIN_BUTTON1 = 2;
const uint PIN_BUTTON2 = 3;
const uint PIN_BUTTON3 = 4;
const uint PIN_BUTTON4 = 5;

const uint DEBOUNCE_TIME_MS = 200;
static uint64_t last_press_time = 0;

media_manager manager;
Wifi_Config wifi_config;

void playback_loop()
{
  printf("Start playback loop\n");
  State& state = State::getInstance();

  while (true)
  {
    if (state.play_song_flag)
    {
      printf("Setting play flag to false\n");
      state.play_song_flag = false;
      manager.play();
    }
    if (state.stop_song_flag)
    {
      printf("Setting stop flag to false\n");
      manager.stop();
      state.stop_song_flag = false;
    }
    if (state.play_next_song_flag)
    {
      printf("Setting play next song flag to false\n");
      state.play_next_song_flag = false;
      manager.next();
    }
    if (state.play_previous_song_flag)
    {
      printf("Setting play previous song flag to false\n");
      state.play_previous_song_flag = false;
      manager.previous();
    }

    if (manager.is_playing())
    {
      manager.currently_playing();
    }
  }
}

void debounce_and_check_buttons()
{
  State &state = State::getInstance();

  bool button1_pressed = !gpio_get(PIN_BUTTON1);
  bool button2_pressed = !gpio_get(PIN_BUTTON2);
  bool button3_pressed = !gpio_get(PIN_BUTTON3);
  bool button4_pressed = !gpio_get(PIN_BUTTON4);

  uint64_t current_time = to_ms_since_boot(get_absolute_time());

  if (current_time - last_press_time > DEBOUNCE_TIME_MS)
  {
    if (button1_pressed)
    {
      printf("Pressed play, setting flag to true\n");
      state.play_song_flag = true;
    }
    else if (button2_pressed)
    {
      printf("Pressed paused, setting flag to true\n");
      state.stop_song_flag = true;
    }
    else if (button3_pressed)
    {
      printf("Pressed next, setting flag to true\n");
      state.play_next_song_flag = true;
    }
    else if (button4_pressed)
    {
      printf("Pressed previous, setting flag to true\n");
      state.play_previous_song_flag = true;
    }
    last_press_time = current_time;
  }
}

void event_loop()
{
  while (true)
  {
    sys_check_timeouts();
    debounce_and_check_buttons();
  }
}

int main()
{
  stdio_init_all();
  cyw43_arch_init();
  // Serial::init();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected());

  std::string ssid;
  std::string password;

  manager.init();

  printf("Attempting to load wifi credentials from flash...\n");
  if (write_flash.load_credentials(ssid, password))
  {
    printf("Loaded credentials, now trying to connect with them %s\n", ssid.c_str());
    if (!Wifi::connect(ssid, password))
    {
      printf("Connect wifi error\n");
    }
  } else {
    printf("The credentials did not match or were not found, setting up access point\n");

    wifi_config.setup_access_point();

    // Initialise web server
    httpd_init();
    printf("Http server initialised\n");
    cgi_init();
    printf("CGI Handler initialised\n");

    State &state = State::getInstance();

    while (state.switch_cyw43_mode == false)
    {
      sys_check_timeouts();
      cyw43_arch_poll();
      sleep_ms(100);
    } 

    printf("Switching to STA mode and attempting to load credentials from flash\n");

    if (write_flash.load_credentials(ssid, password)) {
      printf("Loaded credentials after mode switch, SSID: %s\n", ssid.c_str());
      if (!Wifi::connect(ssid, password))
      {
        printf("Connect wifi error\n");
      }
    } else 
    {
      printf("Failed to load wifi credentials afte mode switch");
    }
  }


  manager.get_playlist();
  manager.get_album_cover("https://marcai.blob.core.windows.net/image/marcai.png");

  gpio_init(PIN_BUTTON1);
  gpio_set_dir(PIN_BUTTON1, GPIO_IN);
  gpio_pull_up(PIN_BUTTON1);

  gpio_init(PIN_BUTTON2);
  gpio_set_dir(PIN_BUTTON2, GPIO_IN);
  gpio_pull_up(PIN_BUTTON2);

  gpio_init(PIN_BUTTON3)
  gpio_set_dir(PIN_BUTTON3, GPIO_IN);
  gpio_pull_up(PIN_BUTTON3);
  
  gpio_init(PIN_BUTTON4);
  gpio_set_dir(PIN_BUTTON4, GPIO_IN);
  gpio_pull_up(PIN_BUTTON4);

  multicore_launch_core1(event_loop);
  playback_loop();
}
