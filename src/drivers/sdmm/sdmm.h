#ifndef SDMM_H
#define SDMM_H

#include "hardware/spi.h"

#define CMD0 (0x00)
#define CMD1 (0x01)
#define CMD8 (0x08)
#define CMD12 (0x0C)
#define CMD16 (0x10)
#define CMD17 (0x11)
#define CMD18 (0x12)
#define CMD55 (0x37)
#define CMD58 (0x3A)
#define ACMD41 (0x29)

#define R1_IDLE_STATE (0x01)
#define R1_ILLEGAL_COMMAND (0x05)
#define R1_READY_STATE (0x00)

void spi_initialize (spi_inst_t *spi, uint8_t CS_PIN, uint8_t MISO_PIN, uint8_t SCK_PIN, uint8_t MOSI_PIN);
uint8_t spi_status();
uint8_t spi_type();

static uint8_t send_cmd (spi_inst_t *spi, uint8_t cmd, uint32_t arg, uint8_t crc);

static uint8_t send_acmd(spi_inst_t *spi, uint8_t command, uint32_t arg, uint8_t crc);

/**
 * @brief Receive a data packet from MMC
 * @param spi SPI instance
 * @param buff Data buffer to store received data
 * @param btr Byte count (must be multiple of 4)
 * @return 1:OK, 0:Failed
 */
uint8_t read (spi_inst_t *spi, uint32_t sector, uint8_t *buffer, uint8_t count);

/**
 * @brief Wait for card ready
 * @param spi SPI instance
 * @return 1:Ready, 0:Timeout
 */
static uint8_t wait_ready(spi_inst_t *spi);
static uint8_t spi_select(spi_inst_t *spi);
static void spi_deselect(spi_inst_t *spi);

/**
 * @brief Receive a data packet from MMC
 * @param spi SPI instance
 * @param buff Data buffer to store received data
 * @param btr Byte count (must be multiple of 4)
 * @return 1:OK, 0:Failed
 */
static uint8_t rcvr_datablock(spi_inst_t *spi, uint8_t *buff, uint32_t btr);

#endif