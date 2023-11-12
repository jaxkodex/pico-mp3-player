#include "video.h"

#include <stdlib.h> // malloc() free()

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in14.h"
#include "Debug.h"
#include "Infrared.h"

#include "../../modules/logger/logger.h"

static UWORD *BlackImage;

static uint16_t background_color = WHITE;

void init_video() {
  if (DEV_Module_Init() != 0) {
    error("Cannot initialize video");
    return;
  }

  DEV_SET_PWM(50);
  /* LCD Init */
  info("1.14inch LCD demo...\r\n");
  LCD_1IN14_Init(HORIZONTAL);
  LCD_1IN14_Clear(WHITE);
  info("Initialized video!");
    
  UDOUBLE Imagesize = LCD_1IN14_HEIGHT*LCD_1IN14_WIDTH*2;
  if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
      error("Failed to apply for black memory...\r\n");
      exit(0);
  }

  // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
  Paint_NewImage((UBYTE *)BlackImage,LCD_1IN14.WIDTH,LCD_1IN14.HEIGHT, 0, WHITE);
  Paint_SetScale(65);
  Paint_Clear(WHITE);
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(WHITE);

  flush();
}


// void render() {
//   // Paint_Clear(background_color);
//   // // Paint_DrawString_EN(pos_x, pos_y, "Hello World", &Font20, 0x000f, 0xfff0);
//   // LCD_1IN14_Display((UWORD *)BlackImage);
// }

void flush() {
  LCD_1IN14_Display((UWORD *)BlackImage);
}

void toggle_background() {
  background_color = background_color == WHITE ? BLACK : WHITE;
}