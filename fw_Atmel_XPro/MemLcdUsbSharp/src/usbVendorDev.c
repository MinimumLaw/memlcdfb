/*
 *
 */

#include "asf.h"
#include <string.h>
#include "usbVendorDev.h"
#include "memlcd.h"

// Receive Buffer
COMPILER_WORD_ALIGNED
static uint8_t usbVendorDevRecvBuff[USB_VENDOR_DEVICE_OUT_EP_SIZE];

// callback functions prototypes
void vendor_out_ep_callback(udd_ep_status_t status,
iram_size_t nb_transfered, udd_ep_id_t ep);

/*
 * Vendor device ennumerated and ready for action
 */
bool usbVendorDeviceEnableExt(void)
{
	// Enable OUT transfer on bulk endpoint
	udi_display_bulk_out_run(
		usbVendorDevRecvBuff, 
		USB_VENDOR_DEVICE_OUT_EP_SIZE, 
		&vendor_out_ep_callback);

	return true;
}

/*
 * Vendor device unconnected from USB host
 */
void usbVendorDeviceDisableExt(void)
{
	// Disable OUT transfer on bulk endpoint
	/* ToDo: Write this code */
}

/*
 * Vendor device specific setup OUT packet
 */
bool usbVendorDeviceSetupOutReceived(void)
{
	return true;
}

/*
 * Vendor device specific setup IN packet
 */
bool usbVendorDeviceSetupInReceived(void)
{
	return true;
}

/*
 * Callback for Vendor device OUT endpoint transfer complite
 */
void vendor_out_ep_callback(udd_ep_status_t status,
			iram_size_t nb_transfered, udd_ep_id_t ep)
{
	if(UDD_EP_TRANSFER_OK != status) return; /* Good transfers only */

	/* ToDo: check transfer size */
	
	if(usbVendorDevRecvBuff[0] > DISPLAY_HEIGH_DOTS) return;
	
	memcpy(memLcdFrameBuffer + usbVendorDevRecvBuff[0] * DISPLAY_WIDTH_BYTES,
		usbVendorDevRecvBuff + 1, DISPLAY_WIDTH_BYTES);
//	memset(memLcdFrameBuffer, 0xa5, DISPLAY_WIDTH_BYTES * DISPLAY_HEIGH_DOTS);

	// Restart OUT transfer on bulk endpoint
	udi_display_bulk_out_run(
	usbVendorDevRecvBuff,
	USB_VENDOR_DEVICE_OUT_EP_SIZE,
	&vendor_out_ep_callback);
}
