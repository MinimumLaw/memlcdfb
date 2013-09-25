/*
 *
 */

#ifndef _INCLUDE_USB_VENDOR_DEVICE_H_
#define _INCLUDE_USB_VENDOR_DEVICE_H_

#define USB_VENDOR_DEVICE_OUT_EP_SIZE	64

/*
 * Class specific functions
 */
extern bool usbVendorDeviceEnableExt(void);
extern void usbVendorDeviceDisableExt(void);
extern bool usbVendorDeviceSetupOutReceived(void);
extern bool usbVendorDeviceSetupInReceived(void);

#endif
