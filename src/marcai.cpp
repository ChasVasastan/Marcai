#include <stdio.h>
#include <vector>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/timeouts.h"
#include "pico/multicore.h"

#include "media_manager.h"
#include "wifi.h"
#include "state.h"
// #include "serial.h"

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

void playback_loop()
{
  State& state = State::getInstance();

  while (true)
  {
    if (state.play_song_flag)
    {
      state.play_song_flag = false;
      manager.play();
    }
    if (state.pause_song_flag)
    {
      if (!manager.pause())
      {
        printf("Paused\n");
      }
      else
      {
        printf("Unpaused\n");
      }
      state.pause_song_flag = false;
    }
    if (state.play_next_song_flag)
    {
      state.play_next_song_flag = false;
      manager.next();
    }
    if (state.play_previous_song_flag)
    {
      state.play_previous_song_flag = false;
      manager.previous();
    }
    sleep_ms(10);
  }
}

void debounce_and_check_buttons()
{
  State& state = State::getInstance();

  bool button1_pressed = !gpio_get(PIN_BUTTON1);
  bool button2_pressed = !gpio_get(PIN_BUTTON2);
  bool button3_pressed = !gpio_get(PIN_BUTTON3);
  bool button4_pressed = !gpio_get(PIN_BUTTON4);

  uint64_t current_time = to_ms_since_boot(get_absolute_time());

  if (current_time - last_press_time > DEBOUNCE_TIME_MS)
  {
    if (button1_pressed)
    {
      printf("Play\n");
      state.play_song_flag = true;
    }
    else if (button2_pressed)
    {
      state.pause_song_flag = true;
    }
    else if (button3_pressed)
    {
      printf("Playing next song\n");
      state.play_next_song_flag = true;
    }
    else if (button4_pressed)
    {
      printf("Playing previous song\n");
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
    sleep_ms(10);
  }
}

int main()
{
  stdio_init_all();
  cyw43_arch_init();
  // Serial::init();

  // The pico will start when you start the terminal
  while (!stdio_usb_connected());

  manager.init();

  if (!Wifi::connect(WIFI_SSID, WIFI_PASS))
  {
    printf("Connect wifi error\n");
  }

  manager.get_playlist();

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

  multicore_launch_core1(playback_loop);
  event_loop();
}
