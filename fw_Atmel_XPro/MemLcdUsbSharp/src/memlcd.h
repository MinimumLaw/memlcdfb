#ifndef _INCLUDE_MEMLCD_H_
#define _INCLUDE_MEMLCD_H_

#include "asf.h"

#define DISPLAY_WIDTH_DOTS		400
#define DISPLAY_HEIGH_DOTS		242

#define DISPLAY_WIDTH_BYTES		(DISPLAY_WIDTH_DOTS>>3)

extern uint8_t memLcdFrameBuffer[];

#pragma pack(push,1)

typedef union {
	struct {
		uint8_t		update:1;
		uint8_t		inversion:1;
		uint8_t		all_clear:1;
		uint8_t		:5;
	} field;
	uint8_t	value;
} mode_flags;

typedef struct {
	mode_flags	mode;
	uint8_t		line;
	uint8_t		data[DISPLAY_WIDTH_BYTES];
	/* uint16_t	dummy; - placed directly in code */
} LCD_DATA_UPDATE_LINE;

#pragma pack(pop)

/*
 * Functions
 */
void memlcd_spi_init(void);
void memcld_write_line(uint16_t line, uint8_t* data);
void memlcd_update_screen(void);

#endif
