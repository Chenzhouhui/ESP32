#ifndef __SPI_H
#define __SPI_H

#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "driver/gpio.h"

#define USER_SPI_HOST      SPI2_HOST
#define SPI_MOSI_PIN       GPIO_NUM_23
#define SPI_MISO_PIN       GPIO_NUM_19
#define SPI_SCLK_PIN       GPIO_NUM_18
#define SPI_CS_PIN         GPIO_NUM_4

#define SPI_CLOCK_HZ       (27 * 1000 * 1000) // 当前 SPI 时钟：27MHz

void spi_init(void);
void spi_write_data(const uint8_t *data, size_t len);
void spi_read_data(uint8_t *rxdata, size_t len);
#endif // __SPI_H