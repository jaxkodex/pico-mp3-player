#include "ui.h"

#include "GUI_Paint.h"

#include "../../drivers/video/video.h"
#include "../../modules/logger/logger.h"

#include "screensaver.h"

ui_state_t ui_state = {
  .background_color = BLACK,
  .foreground_color = MATRIX_GREEN,
  .state = IDLE,
  .fn_render = NULL
};

void render() {
  if (ui_state.state == UI_ERROR) {
    error("The UI is in an error state.");
  }

  Paint_Clear(ui_state.background_color);
  if (ui_state.fn_render != NULL) {
    (*ui_state.fn_render)(ui_state.background_color, ui_state.foreground_color);
  } else {
    error("The UI has no render function.");
  }

  flush();
}

void navigate(ui_state_e next_ui_state) {
  switch (next_ui_state)
  {
  case SCREENSAVER:
    ui_state.state = SCREENSAVER;
    ui_state.fn_render = &render_screensaver;
    break;
  
  default:
    ui_state.state = SCREENSAVER;
    ui_state.fn_render = &render_screensaver;
    break;
  }
}