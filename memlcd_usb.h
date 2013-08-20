/*
 *
 */

#ifndef _INCLUDE_MEMLCD_USB_H_
#define _INCLUDE_MEMLCD_USB_H_

#define LS027B7DH01_WIDTH		(400)
#define LS027B7DH01_HEIGHT		(241)
/* FixMe: We need framebuffer line len multiples 4. Need macro for this. */
#define LS027B7DH01_LINE_LEN		(52)
#define LS027B7DH01_SPI_LINE_LEN	(LS027B7DH01_WIDTH / 8)
#define LS027B7DH01_SCREEN_SIZE \
    (LS027B7DH01_LINE_LEN * LS027B7DH01_HEIGHT)

struct memlcd_usb_dev {
	/* usb specific part */
	struct usb_device	*udev;
	struct usb_interface	*iface;
	struct usb_anchor	submitted;
	__u8			ep;
	/* framebuffer specific part */
	struct fb_info		*info;
	unsigned char		*video_memory;
};

#endif
