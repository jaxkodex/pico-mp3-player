#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "sdmm.h"

typedef struct SpiConfiguration {
  uint8_t cs;
} SpiConfiguration;

static uint8_t Stat = 0x01;	/* Physical drive status */
static uint8_t CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */
static SpiConfiguration spiConfiguration;

uint8_t spi_type() {
  return CardType;
}

uint8_t spi_status() {
  return Stat;
}

void spi_initialize(spi_inst_t *spi, uint8_t CS_PIN, uint8_t MISO_PIN, uint8_t SCK_PIN, uint8_t MOSI_PIN) {
  printf("Initializing SPI mode for SD card...\n");
  printf("Pin configuration\n");

  spiConfiguration.cs = CS_PIN;

  sleep_ms(10);
  gpio_init(CS_PIN);
  // sets the pin in output mode
  gpio_set_dir(CS_PIN, GPIO_OUT); // sdCsInit(m_csPin);
  // sets pin to high
  gpio_put(CS_PIN, 1); // spiUnselect();

  // Configure clock to 400kHz, required for the SD card to work
  spi_init(spi, 400 * 1000); // spiSetSckSpeed(1000UL * SD_MAX_INIT_RATE_KHZ);
  gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);

  // spiStart();
  gpio_put(CS_PIN, 0); //  spiSelect();
  uint8_t d[1] = {0xFF};
  spi_write_blocking(spi, d, 1); //  spiSend(0XFF);
  // end spiStart

  gpio_put(CS_PIN, 1); //  spiUnselect();
  //0XFF
  uint8_t rx1[1];
  uint8_t tx1 = 0xFF;
  for (uint8_t i = 0; i < 10; i++) {
    spi_read_blocking(spi, tx1, rx1, 1);
  }

  gpio_put(CS_PIN, 0); // spiSelect

  printf("Starting\n");

	uint8_t n, ty, cmd, rx[4];
	uint tmr;
	uint8_t s;
  
	ty = 0;
	if (send_cmd(spi, CMD0, 0, 0x95) == 1) {			/* Enter Idle state */
    printf("Idle state\n");
		if (send_cmd(spi, CMD8, 0x1AA, 0x87) == 1) {	/* SDv2? */
      printf("Checking for SDv2\n");
      spi_read_blocking(spi, 0xFF, rx, 4);
			if (rx[2] == 0x01 && rx[3] == 0xAA) {		/* The card can work at vdd range of 2.7-3.6V */
        printf("The card can work at vdd range of 2.7-3.6V\n");
				for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_acmd(spi, ACMD41, 1UL << 30, 0x65) == 0) break;
					sleep_ms(1);
				}
        if (tmr) {
          printf("Checking for SDv2\n");
          if (send_cmd(spi, CMD58, 0, 0x95) == 0) {	/* Check CCS bit in the OCR */
            spi_read_blocking(spi, 0xFF, rx, 4);
            ty = (rx[0] & 0x40) ? 0x08 | 0x10 : 0x08;	/* SDv2+ */
          }
        }
			}
		} else {							/* SDv1 or MMCv3 */
      printf("Checking for SDv1 or MMCv3\n");
			if (send_acmd(spi, ACMD41, 1UL << 30, 0x65) <= 1) 	{
        printf("Type: SDv1\n");
				ty = 0x08; cmd = ACMD41;	/* SDv1 */
			} else {
        printf("Type: MMCv3\n");
				ty = 0x01; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state */
				if (send_cmd(spi, cmd, 0, 0x01) == 0) break;
				sleep_ms(1);
			}
      if (tmr) {
        if (send_cmd(spi, CMD16, 512, 0x01) != 0) {	/* Set R/W block length to 512 */
          printf("Set R/W block length to 512\n");
          ty = 0;
        }
      }
		}
	}

	CardType = ty;
	s = ty ? 0 : 0x01;
	Stat = s;

  if (s == 0) {
    printf("Initialized\n");
    // Configure clock to 400kHz, required for the SD card to work
    // spi_init(spi, 25 * 1000 * 1000);
    printf("Setting baudrate %d\n", spi_set_baudrate(spi, 15 * 1000 * 1000));
  }

  spi_deselect(spi);

  printf("CardType: %d\n", CardType);
  printf("Stat: %d\n", Stat);
  printf("End of initialization\n");
}

static uint8_t send_acmd(spi_inst_t *spi, uint8_t command, uint32_t arg, uint8_t crc) {
  uint8_t n = send_cmd(spi, CMD55, 0, 0x65);
  if (n > 1) {
    return n;
  }
  return send_cmd(spi, command, arg, crc);
}

static uint8_t wait_ready (spi_inst_t *spi) {
  uint8_t rx[1];
  uint32_t tmr;

  for (tmr = 5000; tmr; tmr--) { /* Wait for ready in timeout of 500ms */
    spi_read_blocking(spi, 0xFF, rx, 1);
    if (rx[0] == 0xFF) break;
    sleep_us(100);
  }
  return tmr ? 1 : 0;
}

static uint8_t spi_select(spi_inst_t *spi) {
  gpio_put(spiConfiguration.cs, 0);

  uint8_t rx[1];
  spi_read_blocking(spi, 0xFF, rx, 1);

  if (wait_ready(spi)) {
    return 1;
  }

  spi_deselect(spi);
  return 0;
}

static void spi_deselect(spi_inst_t *spi) {
  gpio_put(spiConfiguration.cs, 1);
  uint8_t rx[1];
  spi_read_blocking(spi, 0xFF, rx, 1);
}

static uint8_t send_cmd(spi_inst_t *spi, uint8_t command, uint32_t arg, uint8_t crc) {
  if (command != CMD12) {
		spi_deselect(spi);
		if (!spi_select(spi)) {
      return 0xFF;
    }
	}
  uint8_t cmd = command | 0x40;
  
  uint8_t rx;
  uint8_t tx[6] = { cmd, (uint8_t)(arg >> 24), (uint8_t)(arg >> 16), (uint8_t)(arg >> 8), (uint8_t)(arg), crc };
  spi_write_blocking(spi, tx, 6);
  if (command == CMD12) {
    spi_read_blocking(spi, 0xFF, &rx, 1);
  }
  uint8_t n = 10;
  do {
    spi_read_blocking(spi, 0xFF, &rx, 1);
  } while ((rx & 0x80) && n-- > 0);

  return rx;
}

uint8_t read (spi_inst_t *spi, uint32_t sector, uint8_t *buffer, uint8_t count) {
  uint8_t cmd = count > 1 ? CMD18 : CMD17;
  // printf("Read start: %d\n", count);
  // printf("cmd: %d, sector: %d, count: %d\n", cmd, sector, count);
  if (send_cmd(spi, cmd, sector, 0x01) == 0) {
    printf("Reading...\n");
    do {
			if (!rcvr_datablock(spi, buffer, 512)) {
        // printf("Failed to read data block\n");
        break;
      }
			buffer += 512;
    } while (--count);
    if (cmd == CMD18) send_cmd(spi, CMD12, 0, 0x01);	/* STOP_TRANSMISSION */
  }
  spi_deselect(spi);
  // printf("Read end: %d\n", count);
  return count ? 1 : 0;
}

static uint8_t rcvr_datablock (spi_inst_t *spi, uint8_t *buff, uint32_t btr) {
	uint8_t d[2];
	uint32_t tmr;

  printf("rcvr_datablock\n");

	for (tmr = 1000; tmr; tmr--) {	/* Wait for data packet in timeout of 100ms */
		spi_read_blocking(spi, 0xFF, d, 1);
		if (d[0] != 0xFF) break;
		sleep_us(100);
	}
  printf("d[0]: %d\n", d[0]);
	if (d[0] != 0xFE) return 0;		/* If not valid data token, return with error */

	spi_read_blocking(spi, 0xFF, buff, btr);			/* Receive the data block into buffer */
	spi_read_blocking(spi, 0xFF, d, 2);					/* Discard CRC */

  printf("Read sucessful\n");

	return 1;						/* Return with success */
}