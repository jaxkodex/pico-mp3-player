#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "drivers/bt/blue.h"
#include "pico/cyw43_arch.h"
#include "ff.h"
#include "music_file.h"
#include "player.h"
#include "drivers/bt/blue_utils.h"
#include "events.h"
#include "logger.h"
#include "drivers/video/video.h"
#include "drivers/video/button.h"
#include "modules/ui/ui.h"

static char * device_addr_string = "20:64:DE:42:EF:59"; // speaker
// static const char * device_addr_string = "E4:5E:1B:C7:D0:B9"; // pixel buds

void main() {
  bool clock_set = set_sys_clock_khz(180000, true);
  stdio_init_all();

  sleep_ms(3000);

  if (!clock_set) {
    error("Cannot set clock rate");
    return;
  }
  info("Seleeping for 5s...");
  sleep_ms(5000);

  if (cyw43_arch_init()) {
    return;
  }

  init_video();
  init_buttons();
  toggle_background();
  navigate(SCREENSAVER);

  // FATFS fs;
  // FRESULT res;

  // res = f_mount(&fs, "", 1);
  // if (res == FR_OK) {
  //   printf("Mounted!\n");
  //   play_next();
  // } else {
  //     printf("Error mounting: %d\n", res);
  // }

  // uint8_t err = blue_initialize();
  // if (!err) {
  //   printf("Blue initialized!\n");
  //   err = hci_power_control(HCI_POWER_ON);
  //   printf("Power control: %d\n", err);
  // }

  q_init();

  uint8_t just_once = 0;
  uint8_t count = 0;
  event_t event;
  while (true) {
    // info("Waiting for event... %d", count);
    // info(".");
    q_next_event(&event);

    count++;
    if (count == 30) {
      if (!just_once) {
        // blue_connect(device_addr_string);
        // read_next_buffer();
        just_once = 1;
      }
      count = 0;
    }

    switch (event)
    {
    case EVENT_READ_MUSIC_FILE:
      // read_next_buffer();
      info("Read music file");
      break;

    case EVENT_BUTTON_A_PRESSED:
      info("A pressed");
      toggle_background();
      break;

    case EVENT_BUTTON_A_RELEASED:
      info("A released");
      toggle_background();
      break;
    
    default:
      sleep_ms(20);
      break;
    }

    event = EVENT_NOOP;

    render();
  }
}
