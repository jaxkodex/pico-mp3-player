#include "files.h"

#include "ff.h"

static FATFS fs;

void init_fs()
{
  FRESULT res;

  res = f_mount(&fs, "", 1);
  if (res == FR_OK) {
    printf("Mounted!\n");
  } else {
      printf("Error mounting: %d\n", res);
  }
}