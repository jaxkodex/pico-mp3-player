#include "button.h"

#include "hardware/gpio.h"

#include "../../modules/logger/logger.h"
#include "../../events.h"

#ifndef  KEY_A_PIN
#define KEY_A_PIN 15
#endif

#ifndef KEY_B_PIN
#define KEY_B_PIN 17
#endif

#ifndef KEY_UP_PIN
#define KEY_UP_PIN 2
#endif

#ifndef KEY_DOWN_PIN
#define KEY_DOWN_PIN 18
#endif

#ifndef KEY_LEFT_PIN
#define KEY_LEFT_PIN 16
#endif

#ifndef KEY_RIGHT_PIN
#define KEY_RIGHT_PIN 20
#endif

#ifndef KEY_CTRL_PIN
#define KEY_CTRL_PIN 3
#endif

static const button_t keyA = { .pin = KEY_A_PIN };
static const button_t keyB = { .pin = KEY_B_PIN };
static button_t up = { .pin = KEY_UP_PIN };
static button_t down = { .pin = KEY_DOWN_PIN };
static button_t left = { .pin = KEY_LEFT_PIN };
static button_t right = { .pin = KEY_RIGHT_PIN };
static button_t ctrl = { .pin = KEY_CTRL_PIN };

// Each button fires a bit in the order 
// A, B, UP, DOWN, LEFT, RIGHT, CTRL
// MSB will always be 0
static uint8_t button_state = 0;

void init_buttons() {

  gpio_set_irq_enabled_with_callback(keyA.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  gpio_set_irq_enabled(keyB.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(up.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(down.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(left.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(right.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(ctrl.pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

  gpio_pull_up(keyA.pin);
  gpio_pull_up(keyB.pin);
  gpio_pull_up(up.pin);
  gpio_pull_up(down.pin);
  gpio_pull_up(left.pin);
  gpio_pull_up(right.pin);
  gpio_pull_up(ctrl.pin);
  info("Initialized buttons!");

}

static void gpio_callback(unsigned int gpio, uint32_t events) {
  if (events == GPIO_IRQ_EDGE_RISE)
  {
    info("Button %d pressed", gpio);
    event_t event = EVENT_BUTTON_A_PRESSED;
    q_emit(&event);
    return;
  }
  switch (gpio)
  {
  case KEY_A_PIN:
    button_state |= 0b01000000;
    break;
  case KEY_B_PIN:
    button_state |= 0b00100000;
    break;
  case KEY_UP_PIN:
    button_state |= 0b00010000;
    break;
  case KEY_DOWN_PIN:
    button_state |= 0b00001000;
    break;
  case KEY_LEFT_PIN:
    button_state |= 0b00000100;
    break;
  case KEY_RIGHT_PIN:
    button_state |= 0b00000010;
    break;
  case KEY_CTRL_PIN:
    button_state |= 0b00000001;
    break;
  
  default:
    break;
  }
}