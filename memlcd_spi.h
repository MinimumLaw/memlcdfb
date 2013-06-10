/*
 *
 *
 *
 *
 */

#ifndef _INCLUDE_MEMLCD_SPI_H_
#define _INCLUDE_MEMLCD_SPI_H_

#define LS027B7DH01_WIDTH	(400)
#define LS027B7DH01_HEIGHT	(241)
#define LS027B7DH01_LINE_LEN	(56)
#define LS027B7DH01_SPI_LINE_LEN	(50)
#define LS027B7DH01_SCREEN_SIZE	(LS027B7DH01_LINE_LEN * LS027B7DH01_HEIGHT)
//#define LS027B7DH01_

typedef struct {
	unsigned char	cmd;
	unsigned char	addr;
	unsigned char	data[LS027B7DH01_SPI_LINE_LEN];
	unsigned char   dummy[2];
} memlcd_line_update;

typedef struct {
	struct spi_device	*spi;
	struct fb_info		*info;
	unsigned char		*video_memory;
} memlcd_priv;

#endif