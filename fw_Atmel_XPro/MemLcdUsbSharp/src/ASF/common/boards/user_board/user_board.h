/**
 * \file
 *
 * \brief User board definition template
 *
 */

 /* This file is intended to contain definitions and configuration details for
 * features and devices that are available on the board, e.g., frequency and
 * startup time for an external crystal, external memory devices, LED and USART
 * pins.
 */

#ifndef USER_BOARD_H
#define USER_BOARD_H

#include <conf_board.h>

/* LCD SPI interface */
#define LCD_SCS			IOPORT_CREATE_PIN(PIOA, 11)
#define LCD_SI			IOPORT_CREATE_PIN(PIOA, 13)
#define LCD_SCLK		IOPORT_CREATE_PIN(PIOA, 14)
/* LCD Control interface */
#define LCD_EXTCOMIN	IOPORT_CREATE_PIN(PIOA, 1)
#define LCD_DISPLAY		IOPORT_CREATE_PIN(PIOA, 3)
#define LCD_EXTMODE		IOPORT_CREATE_PIN(PIOA, 4)
/* SPI ChipSelect for LCD */
#define LCD_CS			0

/* Board specific clock sources */
#define BOARD_OSC_STARTUP_US			50000L
#define BOARD_FREQ_SLCK_XTAL			32768L
#define BOARD_FREQ_SLCK_BYPASS			32768L
#define BOARD_FREQ_MAINCK_XTAL			12000000L
#define BOARD_FREQ_MAINCK_BYPASS		12000000L

#endif // USER_BOARD_H
