#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <stdint.h>
#include <stdbool.h>

#include "ff.h"
#include "music_file.h"

typedef enum {
  PLAYER_STATE_IDLE,
  PLAYER_STATE_UNINITIALIZED,
  PLAYER_STATE_STOPPED,
  PLAYER_STATE_PLAYING,
  PLAYER_STATE_PAUSED,
  PLAYER_STATE_ERROR
} player_state_e;

typedef struct {
  player_state_e status;
  char current_path[255];
  DIR dir;
  FILINFO fno;
  music_file file_playing;
} player_state_t;

void play_next();
void blue_produce_audio(int16_t * pcm_buffer, int num_samples_to_write);
void read_next_buffer();

static uint8_t initialize_player();
static uint8_t find_next_file();

#endif