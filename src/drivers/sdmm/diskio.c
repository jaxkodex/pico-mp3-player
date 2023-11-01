#include <stdio.h>
#include "ff.h"
#include "diskio.h"
#include "sdmm.h"
#include "sdconf.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC3		0x01		/* MMC ver 3 */
#define CT_MMC4		0x02		/* MMC ver 4+ */
#define CT_MMC		0x03		/* MMC */
#define CT_SDC1		0x04		/* SD ver 1 */
#define CT_SDC2		0x08		/* SD ver 2+ */
#define CT_SDC		0x0C		/* SD */
#define CT_BLOCK	0x10		/* Block addressing */

DSTATUS disk_initialize (BYTE pdrv) {
  if (pdrv) return STA_NOINIT; // Supports only single drive

  sleep_ms(10);

  spi_initialize(spi0, CS_PIN, MISO_PIN, SCK_PIN, MOSI_PIN);

  return spi_status();
}

DSTATUS disk_status (BYTE pdrv) {
  if (pdrv) return STA_NOINIT; // Supports only single drive

  return spi_status();
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
  if (pdrv || !count) return RES_PARERR; // Check parameter
  if (spi_status() & STA_NOINIT) return RES_NOTRDY; // Check if drive is ready

  if (!(spi_type() & CT_BLOCK)) sector *= 512; // Convert to byte address if needed

  uint8_t result = read(spi0, sector, buff, count);

  printf("Read result: %d\n", result);

  for (size_t i = 0; i < sizeof(buff); i++)
  {
    printf("buff[%d]: %d\n", i, buff[i]);
  }

  return result ? RES_ERROR : RES_OK;
}