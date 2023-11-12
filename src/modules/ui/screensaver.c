#include "screensaver.h"

#include <stdint.h>

#include "LCD_1in14.h"
#include "GUI_Paint.h"

#include "ui.h"

typedef enum {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction_t;

static uint8_t pos_x = 0, pos_y =  0;
// static uint8_t pos_x = 75, pos_y =  100;
static direction_t direction_x = RIGHT;
static direction_t direction_y = DOWN;

static const char *text[] = {
  "Hello World",
  "Ballx NerXe",
  "HRt#a 6>&ld",
  "RRG#a 6>&Cd",
  "HRG#a 6o&Cd",
  "HRlla 6o&ld",
  "Hella Wo&ld",
  "Hello Wo&ld",
};

static uint8_t text_index = 0;

void render_screensaver(uint16_t background_color, uint16_t foreground_color) {
  if (direction_x == RIGHT && direction_y == DOWN) {
    pos_x++;
    pos_y++;
  }
  if (direction_x == LEFT && direction_y == DOWN) {
    pos_x--;
    pos_y++;
  }
  if (direction_x == RIGHT && direction_y == UP) {
    pos_x++;
    pos_y--;
  }
  if (direction_x == LEFT && direction_y == UP) {
    pos_x--;
    pos_y--;
  }

  if (pos_x > LCD_1IN14.WIDTH -155) {
    direction_x = LEFT;
  }
  if (pos_x < 1) {
      direction_x = RIGHT;
  }
  if (pos_y > LCD_1IN14.HEIGHT -21) {
    direction_y = UP;
  }
  if (pos_y < 1) {
    direction_y = DOWN;
  }

  Paint_DrawString_EN(pos_x, pos_y, text[text_index], &Font20, BLACK, MATRIX_GREEN);
  text_index++;
  if (text_index > 7) {
    text_index = 0;
  }
}