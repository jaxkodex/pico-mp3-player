#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ff.h"

FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
  printf("Scanning files...\n");
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
      printf("Opened directory\n");
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                printf("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}

void main() {
  stdio_init_all();
  sleep_ms(20000);

  FATFS fs;
  FRESULT res;
  char buff[256];

  res = f_mount(&fs, "", 1);
  if (res == FR_OK) {
      printf("Mounted!\n");
      strcpy(buff, "/");
      res = scan_files(buff);
  } else {
      printf("Error mounting: %d\n", res);
  }
  
  while (true) {
      printf("Muriox!\n");
      sleep_ms(1000);
  }
}