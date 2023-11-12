#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <stdint.h>

typedef struct {
  uint8_t pin;
} button_t;

void init_buttons();

static void gpio_callback(unsigned int gpio, uint32_t events);

#endif