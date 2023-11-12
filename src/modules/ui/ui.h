#ifndef __UI_H__
#define __UI_H__

#include <stdint.h>

#define MATRIX_GREEN 0x04EB

typedef enum {
  UI_ERROR,
  IDLE,
  HOME,
  SCREENSAVER
} ui_state_e;

typedef struct {
  ui_state_e state;
  uint16_t background_color;
  uint16_t foreground_color;
  void (*fn_render)(uint16_t background_color, uint16_t foreground_color);
} ui_state_t;

void render();
void navigate(ui_state_e ui_state);

#endif