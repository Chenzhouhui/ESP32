#include "spi.h"

static spi_device_handle_t spi_handle;
void spi_init(void)
{
    spi_bus_config_t spi_init_struct = {
        .mosi_io_num = SPI_MOSI_PIN,
        .miso_io_num = SPI_MISO_PIN,
        .sclk_io_num = SPI_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 16 * 1024,
        .flags = 0, // No special flags
        .isr_cpu_id = 0, // Use default CPU for SPI ISR
        .intr_flags = 0 // No special interrupt flags
    };
    spi_bus_initialize(USER_SPI_HOST, &spi_init_struct, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t spi_device_config = {
        .clock_speed_hz = SPI_CLOCK_HZ, // Set the SPI clock speed
        .mode = 0, // SPI mode 0
        .queue_size = 8,
        .spics_io_num = SPI_CS_PIN,
        .pre_cb = NULL,
    };
    spi_bus_add_device(USER_SPI_HOST, &spi_device_config, &spi_handle);

}

void spi_write_data(const uint8_t *data, size_t len)
{
    spi_transaction_t transaction = {
        .length = len * 8,
        .flags = 0,
        .rx_buffer = NULL,
        .tx_buffer = data
    };
    spi_device_transmit(spi_handle, &transaction);
}
void spi_read_data(uint8_t *rxdata, size_t len)
{
    spi_transaction_t transaction = {
        .length = len * 8,
        .flags = 0,
        .rx_buffer = rxdata,
        .tx_buffer = NULL
    };
    spi_device_transmit(spi_handle, &transaction);
}