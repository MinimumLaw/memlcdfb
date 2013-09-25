/**
 * \file
 *
 * \brief User board initialization template
 *
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>
#include "memlcd.h"

void memlcd_spi_init(void)
{
	/* configure LCD control pins */
	ioport_set_pin_dir(LCD_DISPLAY, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(LCD_DISPLAY, true);
	ioport_set_pin_dir(LCD_EXTCOMIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(LCD_EXTCOMIN, false);
	ioport_set_pin_dir(LCD_EXTMODE, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(LCD_EXTMODE, false);
	ioport_set_pin_dir(LCD_SCS, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(LCD_SCS, false);
	/* configure LCD SPI interface pins */
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA13A_MOSI);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA14A_SPCK);
	/* configure SPI interface mode */
	spi_enable_clock(SPI);
	spi_reset(SPI);
	spi_set_master_mode(SPI);
	spi_disable_mode_fault_detect(SPI);
	spi_disable_loopback(SPI);
	spi_set_peripheral_chip_select_value(SPI, LCD_CS);
	spi_set_fixed_peripheral_select(SPI);
	spi_disable_peripheral_select_decode(SPI);
	spi_disable_tx_on_rx_empty(SPI);
	spi_set_delay_between_chip_select(SPI, 128);
	/* configure specific SPI chip select configuration */
	spi_set_transfer_delay(SPI, LCD_CS, 0, 0);
	spi_set_bits_per_transfer(SPI, LCD_CS, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI, LCD_CS, 
		spi_calc_baudrate_div(2000000,sysclk_get_cpu_hz()));
	spi_configure_cs_behavior(SPI, LCD_CS, SPI_CS_RISE_FORCED);
	spi_set_clock_polarity(SPI, LCD_CS, LOW);
	spi_set_clock_phase(SPI, LCD_CS, 0);
	spi_enable(SPI);
}

void board_init(void)
{
	sysclk_init();
	sleepmgr_init();
	ioport_init();
	pmc_enable_periph_clk(ID_PIOA);
	pmc_enable_periph_clk(ID_SPI);
	memlcd_spi_init();
	udc_start();
	udc_attach();
	wdt_init(WDT, WDT_MR_WDRSTEN, 4095, 4095);	/* or wdt_disable(WDT); */
}
