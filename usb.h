/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

#ifndef __RTK_USB_H_
#define __RTK_USB_H_


#define RTK_USB_DEVICE(vend, dev, hw_config)	\
	USB_DEVICE(vend, dev),			\
	.driver_info = (kernel_ulong_t) & (hw_config),

#define RTK_USB_DEVICE_AND_INTERFACE(vend, dev, cl, sc, pr, hw_config)	\
	USB_DEVICE_AND_INTERFACE_INFO(vend, dev, cl, sc, pr),		\
	.driver_info = (kernel_ulong_t) & (hw_config),

#define REALTEK_USB_VENQT_MAX_BUF_SIZE		254
#define FW_8192C_START_ADDRESS			0x1000
#define FW_8192C_END_ADDRESS			0x5FFF
#define RTL_USB_MAX_RX_COUNT     		100

struct rtw_usb {
	struct usb_device *udev;
	spinlock_t usb_lock;

	__le32 *usb_data;
	int usb_data_index;

	int ep_out[RTK_MAX_TX_QUEUE_NUM];
	int ep_in;
};
#endif
