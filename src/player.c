#include <stdio.h>

#include "picomp3libconf.h"
#include "player.h"
#include "events.h"

static player_state_t player_state = {
  .status = PLAYER_STATE_UNINITIALIZED
};

#define music_buffer_size 16000

static uint8_t working_buffer[2000];
static uint16_t music_data1[music_buffer_size];
static uint16_t music_data2[music_buffer_size];
static uint32_t music_data1_len = 0;
static uint32_t music_data2_len = 0;

void play_next() {
  if (player_state.status == PLAYER_STATE_ERROR) {
    printf("Error playing next file, player is in error state.\n");
    return;
  }
  uint8_t err;
  char file_name_buff[256];
  if (player_state.status == PLAYER_STATE_UNINITIALIZED) {
    err = initialize_player();
  } else {
    err = find_next_file();
  }

  if (err) {
    player_state.status = PLAYER_STATE_ERROR;
    printf("Error playing next file, player is in error state.\n");
    return;
  }

  sprintf(&file_name_buff[0], "%s/%s", player_state.current_path, player_state.fno.fname);
  printf("Opening music file %s\n", file_name_buff);
  bool success = musicFileCreate(&player_state.file_playing, file_name_buff, working_buffer, sizeof(working_buffer));
  if (!success) {
    player_state.status = PLAYER_STATE_ERROR;
    printf("Error creating music file.\n");
    return;
  }
  player_state.status = PLAYER_STATE_PLAYING;
  printf("Playing %s/%s\n", player_state.current_path, player_state.fno.fname);

  uint32_t sample_rate;
  bool sampled_stereo;
  sample_rate = musicFileGetSampleRate(&player_state.file_playing);
  sampled_stereo = musicFileIsStereo(&player_state.file_playing);
  printf("Sample rate: %d\n", sample_rate);
  printf("Sampled stereo: %d\n", sampled_stereo);

  musicFileRead(&player_state.file_playing, music_data1, music_buffer_size, &music_data1_len);
  musicFileRead(&player_state.file_playing, music_data2, music_buffer_size, &music_data2_len);
  printf("Read %d samples buffer 1 out of %d.\n", music_data1_len, music_buffer_size);
  printf("Read %d samples buffer 2 out of %d.\n", music_data2_len, music_buffer_size);

  for (uint8_t i = 0; i < 10; i++) {
    printf("0x%02x 0x%02x ", music_data1[i<<1], music_data1[(i<<1) + 1]);
  }
  printf("\n");
  for (uint8_t i = 0; i < 10; i++) {
    printf("0x%02x 0x%02x ", music_data2[i<<1], music_data2[(i<<1) + 1]);
  }
}

static uint8_t initialize_player() {
  FRESULT res;
  sprintf(&player_state.current_path[0], "%s", "");

  res = f_opendir(&player_state.dir, player_state.current_path);
  if (res == FR_OK) {
    printf("Opened directory\n");
    uint8_t err = find_next_file();
    if (err == 0) {
      player_state.status = PLAYER_STATE_IDLE;
      return 0;
    } else {
      player_state.status = PLAYER_STATE_ERROR;
      return 1;
    }
  } else {
    printf("Error opening directory: %d\n", res);
    player_state.status = PLAYER_STATE_ERROR;
    return 1;
  }
}

static uint8_t find_next_file() {
  FRESULT res;
  UINT i;

  res = f_readdir(&player_state.dir, &player_state.fno);
  if (res != FR_OK) return 1;
  if (player_state.fno.fname[0] == 0) {
    f_closedir(&player_state.dir);
  }
  while (player_state.fno.fattrib & AM_DIR) {
    res = f_readdir(&player_state.dir, &player_state.fno);
    if (res != FR_OK) return 1;
  }

  if (player_state.fno.fname[0] == 0) {
    f_closedir(&player_state.dir);
    return 2;
  }
  printf("%s/%s\n", player_state.current_path, player_state.fno.fname);
  return 0;
}

static int audio_position = 0;
static uint16_t * music_data = music_data1;
static bool current_buffer_is_1 = true;
void blue_produce_audio(int16_t * pcm_buffer, int num_samples_to_write) {
  int count;
  uint16_t left;
  uint16_t right;
  uint32_t current_size = current_buffer_is_1 ? music_data1_len : music_data2_len;

  // printf("Producing %d samples\n", num_samples_to_write);
  uint32_t ram_buffer_wrap = current_size<<1;

  for (count = 0; count < num_samples_to_write; count++) {
    left = (music_data[audio_position <<1]);
    right = (music_data[(audio_position <<1) +1]);
    pcm_buffer[count<<1] = left;
    pcm_buffer[(count<<1) + 1] = right;

    // printf("at %d: 0x%02x 0x%02x\n", audio_position, pcm_buffer[count*2], pcm_buffer[count*2 + 1]);

    audio_position++;

    if (audio_position<<1 == ram_buffer_wrap) {
      if (current_buffer_is_1) {
        music_data = music_data2;
        current_buffer_is_1 = false;
      } else {
        music_data = music_data1;
        current_buffer_is_1 = true;
      }
      audio_position = 0;

      event_t event = EVENT_READ_MUSIC_FILE;
      q_emit(&event);
    }
  }

  // printf("Produced %d samples\n", count);
}

void read_next_buffer() {
  uint16_t *music_data;
  uint32_t *read_len;

  printf("Refill\n");

  if (current_buffer_is_1) {
    music_data = music_data2;
    read_len = &music_data2_len;
  } else {
    music_data = music_data1;
    read_len = &music_data1_len;
  }
  musicFileRead(&player_state.file_playing, music_data, music_buffer_size, read_len);
  printf("Read %d samples buffer %d out of %d.\n", *read_len, current_buffer_is_1 ? 2 : 1, music_buffer_size);
  for (uint8_t i = 0; i < 10; i++) {
    printf("0x%02x 0x%02x ", music_data[i<<1], music_data[(i<<1) + 1]);
  }
}