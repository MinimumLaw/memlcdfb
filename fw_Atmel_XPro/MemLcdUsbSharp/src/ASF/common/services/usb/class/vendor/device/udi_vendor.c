/**
 * \file
 *
 * \brief USB Vendor class interface.
 *
 * Copyright (c) 2011 - 2012 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "conf_usb.h"
#include "usb_protocol.h"
#include "usb_protocol_vendor.h"
#include "udd.h"
#include "udc.h"
#include "udi_vendor.h"
#include <string.h>

// Configuration check
#ifndef UDI_DISPLAY_ENABLE_EXT
# error UDI_DISPLAY_ENABLE_EXT must be defined in conf_usb.h file.
#endif
#ifndef UDI_DISPLAY_DISABLE_EXT
# error UDI_DISPLAY_DISABLE_EXT must be defined in conf_usb.h file.
#endif

/**
 * \ingroup udi_vendor_group
 * \defgroup udi_vendor_group_udc Interface with USB Device Core (UDC)
 *
 * Structures and functions required by UDC.
 *
 * @{
 */
bool udi_display_enable(void);
void udi_display_disable(void);
bool udi_display_setup(void);
uint8_t udi_display_getsetting(void);

//! Global structure which contains standard UDI API for UDC
UDC_DESC_STORAGE udi_api_t udi_api_display = {
	.enable = udi_display_enable,
	.disable = udi_display_disable,
	.setup = udi_display_setup,
	.getsetting = udi_display_getsetting,
	.sof_notify = NULL,
};
//@}


/**
 * \ingroup udi_vendor_group
 * \defgroup udi_vendor_group_internal Implementation of UDI Vendor Class
 *
 * Class internal implementation
 * @{
 */

//! USB descriptor alternate setting used
static uint8_t udi_display_alternate_setting = 0;

/**
 * \name Internal routines
 */
//@{
bool udi_display_enable(void)
{
	udi_display_alternate_setting = udc_get_interface_desc()->bAlternateSetting;
	if (0 == udi_display_alternate_setting) {
		// Call application callback
		// to notify that interface is enabled
		if (!UDI_DISPLAY_ENABLE_EXT()) {
			return false;
		}
	}
	return true;
}


void udi_display_disable(void)
{
	if (1 == udi_display_alternate_setting) {
		UDI_DISPLAY_DISABLE_EXT();
	}
}


bool udi_display_setup(void)
{
	if (Udd_setup_is_in()) {
		if ((Udd_setup_type() == USB_REQ_TYPE_VENDOR)
				&& (udd_g_ctrlreq.req.bRequest == 0)) {
			return UDI_DISPLAY_SETUP_IN_RECEIVED();
		}
	}
	if (Udd_setup_is_out()) {
		if ((Udd_setup_type() == USB_REQ_TYPE_VENDOR)
				&& (udd_g_ctrlreq.req.bRequest == 0)
				&& (0 != udd_g_ctrlreq.req.wLength)) {
			return UDI_DISPLAY_SETUP_OUT_RECEIVED();
		}
	}
	return false; // Not supported request
}

uint8_t udi_display_getsetting(void)
{
	return udi_display_alternate_setting;
}
//@}

#if UDI_DISPLAY_EPS_SIZE_BULK_FS
/**
 * \brief Start a transfer on bulk OUT
 *
 * When the transfer is finished or aborted (stall, reset, ...), the \a callback is called.
 * The \a callback returns the transfer status and eventually the number of byte transfered.
 *
 * \param buf           Buffer on Internal RAM to send or fill.
 *                      It must be align, then use COMPILER_WORD_ALIGNED.
 * \param buf_size      Buffer size to send or fill
 * \param callback      NULL or function to call at the end of transfer
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
bool udi_display_bulk_out_run(uint8_t * buf, iram_size_t buf_size,
		udd_callback_trans_t callback)
{
	return udd_ep_run(UDI_DISPLAY_EP_BULK_OUT,
			false,
			buf,
			buf_size,
			callback);
}
#endif

//@}
