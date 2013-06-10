/*
 *
 *
 *
 *
 */

#ifndef _INCLUDE_MEMLCD_SPI_H_
#define _INCLUDE_MEMLCD_SPI_H_

#define LS027B7DH01_WIDTH		(400)
#define LS027B7DH01_HEIGHT		(241)
/* FixMe: We need framebuffer line len multiples 4. Need macro for this. */
#define LS027B7DH01_LINE_LEN		(52)
#define LS027B7DH01_SPI_LINE_LEN	(LS027B7DH01_WIDTH / 8)
#define LS027B7DH01_SCREEN_SIZE		(LS027B7DH01_LINE_LEN * LS027B7DH01_HEIGHT)
//#define LS027B7DH01_

#pragma pack(push,1)
typedef struct {
	unsigned char	cmd;
	unsigned char	addr;
	unsigned char	data[LS027B7DH01_SPI_LINE_LEN];
} memlcd_line_update;
#pragma pack(pop)

typedef struct {
	struct spi_device	*spi;
	struct fb_info		*info;
	unsigned char		*video_memory;
	void			*spi_buf;
} memlcd_priv;

#endif