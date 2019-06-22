/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

#ifndef __RTK_USB_H_
#define __RTK_USB_H_


#define RTK_USB_DEVICE(vend, dev, hw_config)	\
	USB_DEVICE(vend, dev),			\
	.driver_info = (kernel_ulong_t) & (hw_config),

#define RTK_USB_DEVICE_AND_INTERFACE(vend, dev, cl, sc, pr, hw_config)	\
	USB_DEVICE_AND_INTERFACE_INFO(vend, dev, cl, sc, pr),		\
	.driver_info = (kernel_ulong_t) & (hw_config),

struct rtw_usb {
	struct usb_device *udev;
};
#endif
