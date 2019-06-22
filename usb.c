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

static const struct usb_device_id rtw_usb_id_table[] = {
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
