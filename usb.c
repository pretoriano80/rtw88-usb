// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

#include <linux/module.h>
#include <linux/usb.h>
#include "main.h"
#include "usb.h"
#include "tx.h"
#include "rx.h"
#include "debug.h"


static int rtw_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	int ret = -ENODEV;

	return ret;
}

static void rtw_usb_disconnect(struct usb_interface *intf)
{
}

#define USB_VENDOR_ID_REALTEK           0x0bda
#define USB_VENDOR_ID_EDIMAX		0x7392

static const struct usb_device_id rtw_usb_id_table[] = {
#ifdef CONFIG_RTW88_8822BU
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xB82C, 0xff, 0xff, 0xff, rtw8822b_hw_spec) }, /* Default ID for USB multi-function */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xB812, 0xff, 0xff, 0xff, rtw8822b_hw_spec) }, /* Default ID for USB Single-function, WiFi only */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_EDIMAX, 0xB822, 0xff, 0xff, 0xff, rtw8822b_hw_spec) }, //EDX
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_EDIMAX, 0xC822, 0xff, 0xff, 0xff, rtw8822b_hw_spec) }, //EDX
	{RTK_USB_DEVICE(0x0b05, 0x184c, rtw8822b_hw_spec) },	/* ASUS AC53 Nano */
	{RTK_USB_DEVICE(0x0b05, 0x1841, rtw8822b_hw_spec) },	/* ASUS AC55 B1 */
	{RTK_USB_DEVICE(0x2001, 0x331c, rtw8822b_hw_spec) },	/* D-Link DWA-182 rev D1 */
	{RTK_USB_DEVICE(0x13b1, 0x0043, rtw8822b_hw_spec) },	/* Linksys WUSB6400M */
#endif

#ifdef CONFIG_RTW88_8822CU
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xb82b, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xb820, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xC821, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xC820, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xC82A, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
	{RTK_USB_DEVICE_AND_INTERFACE(USB_VENDOR_ID_REALTEK, 0xC82B, 0xff, 0xff, 0xff, rtw8822c_hw_spec) }, /* 8821CU */
#endif
	{},
};
MODULE_DEVICE_TABLE(usb, rtw_usb_id_table);

static struct usb_driver rtw_usb_driver = {
	.name = "rtw_usb",
	.id_table = rtw_usb_id_table,
	.probe = rtw_usb_probe,
	.disconnect = rtw_usb_disconnect,
};
module_usb_driver(rtw_usb_driver);

MODULE_DESCRIPTION("Realtek 802.11ac wireless USB driver");
MODULE_LICENSE("Dual BSD/GPL");
