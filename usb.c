// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

#include <linux/module.h>
#include <linux/usb.h>
#include "main.h"
#include "usb.h"
#include "tx.h"
#include "rx.h"
#include "debug.h"

#define	REALTEK_USB_VENQT_READ			0xC0
#define	REALTEK_USB_VENQT_WRITE			0x40
#define REALTEK_USB_VENQT_CMD_REQ		0x05
#define	REALTEK_USB_VENQT_CMD_IDX		0x00

#define MAX_USBCTRL_VENDORREQ_TIMES		10

#define _RTK_EP_OUT_MIN				2
#define _RTK_EP_OUT_MAX				4

static void usbctrl_async_callback(struct urb *urb)
{
	if (urb) {
		/* free dr */
		kfree(urb->setup_packet);
		/* free databuf */
		kfree(urb->transfer_buffer);
	}
}

static int _usbctrl_vendorreq_async_write(struct usb_device *udev, u8 request,
					  u16 value, u16 index, void *pdata,
					  u16 len)
{
	int rc;
	unsigned int pipe;
	u8 reqtype;
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	const u16 databuf_maxlen = REALTEK_USB_VENQT_MAX_BUF_SIZE;
	u8 *databuf;

	if (WARN_ON_ONCE(len > databuf_maxlen))
		len = databuf_maxlen;

	pipe = usb_sndctrlpipe(udev, 0); /* write_out */
	reqtype =  REALTEK_USB_VENQT_WRITE;

	dr = kzalloc(sizeof(*dr), GFP_ATOMIC);
	if (!dr)
		return -ENOMEM;

	databuf = kzalloc(databuf_maxlen, GFP_ATOMIC);
	if (!databuf) {
		kfree(dr);
		return -ENOMEM;
	}

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		kfree(databuf);
		kfree(dr);
		return -ENOMEM;
	}

	dr->bRequestType = reqtype;
	dr->bRequest = request;
	dr->wValue = cpu_to_le16(value);
	dr->wIndex = cpu_to_le16(index);
	dr->wLength = cpu_to_le16(len);
	/* data are already in little-endian order */
	memcpy(databuf, pdata, len);
	usb_fill_control_urb(urb, udev, pipe,
			     (unsigned char *)dr, databuf, len,
			     usbctrl_async_callback, NULL);
	rc = usb_submit_urb(urb, GFP_ATOMIC);
	if (rc < 0) {
		kfree(databuf);
		kfree(dr);
	}
	usb_free_urb(urb);
	return rc;
}

static int _usbctrl_vendorreq_sync_read(struct usb_device *udev, u8 request,
					u16 value, u16 index, void *pdata,
					u16 len)
{
	unsigned int pipe;
	int status;
	u8 reqtype;
	int vendorreq_times = 0;
	static int count;

	pipe = usb_rcvctrlpipe(udev, 0); /* read_in */
	reqtype =  REALTEK_USB_VENQT_READ;

	do {
		status = usb_control_msg(udev, pipe, request, reqtype, value,
					 index, pdata, len, 1000);
		if (status < 0) {
			/* firmware download is checksumed, don't retry */
			if ((value >= FW_8192C_START_ADDRESS &&
			    value <= FW_8192C_END_ADDRESS))
				break;
		} else {
			break;
		}
	} while (++vendorreq_times < MAX_USBCTRL_VENDORREQ_TIMES);

	if (status < 0 && count++ < 4)
		pr_err("reg 0x%x, usbctrl_vendorreq TimeOut! status:0x%x value=0x%x\n",
		       value, status, *(u32 *)pdata);
	return status;
}

static u32 _usb_read_sync(struct rtw_dev *rtwdev, u32 addr, u16 len)
{
	struct rtw_usb *rtwusb = (struct rtw_usb *)rtwdev->priv;
	struct usb_device *udev = rtwusb->udev;
	u8 request;
	u16 wvalue;
	u16 index;
	__le32 *data;
	unsigned long flags;

	spin_lock_irqsave(&rtwusb->usb_lock, flags);
	if (++rtwusb->usb_data_index >= RTL_USB_MAX_RX_COUNT)
		rtwusb->usb_data_index = 0;
	data = &rtwusb->usb_data[rtwusb->usb_data_index];
	spin_unlock_irqrestore(&rtwusb->usb_lock, flags);
	request = REALTEK_USB_VENQT_CMD_REQ;
	index = REALTEK_USB_VENQT_CMD_IDX; /* n/a */

	wvalue = (u16)addr;

	_usbctrl_vendorreq_sync_read(udev, request, wvalue, index, data, len);
	return le32_to_cpu(*data);
}

static u8 rtw_usb_read8(struct rtw_dev *rtwdev, u32 addr)
{
	return (u8)_usb_read_sync(rtwdev, addr, 1);
}

static u16 rtw_usb_read16(struct rtw_dev *rtwdev, u32 addr)
{
	return (u16)_usb_read_sync(rtwdev, addr, 2);
}

static u32 rtw_usb_read32(struct rtw_dev *rtwdev, u32 addr)
{
	return _usb_read_sync(rtwdev, addr, 4);
}

static void _usb_write_async(struct usb_device *udev, u32 addr, u32 val,
			     u16 len)
{
	u8 request;
	u16 wvalue;
	u16 index;
	__le32 data;

	request = REALTEK_USB_VENQT_CMD_REQ;
	index = REALTEK_USB_VENQT_CMD_IDX; /* n/a */
	wvalue = (u16)(addr&0x0000ffff);
	data = cpu_to_le32(val);
	_usbctrl_vendorreq_async_write(udev, request, wvalue, index, &data,
				       len);
}

static void rtw_usb_write8(struct rtw_dev *rtwdev, u32 addr, u8 val)
{
	struct rtw_usb *rtwusb = (struct rtw_usb *)rtwdev->priv;

	_usb_write_async(rtwusb->udev, addr, val, 1);
}

static void rtw_usb_write16(struct rtw_dev *rtwdev, u32 addr, u16 val)
{
	struct rtw_usb *rtwusb = (struct rtw_usb *)rtwdev->priv;

	_usb_write_async(rtwusb->udev, addr, val, 2);
}

static void rtw_usb_write32(struct rtw_dev *rtwdev, u32 addr, u32 val)
{
	struct rtw_usb *rtwusb = (struct rtw_usb *)rtwdev->priv;

	_usb_write_async(rtwusb->udev, addr, val, 4);
}
static int rtw_usb_tx(struct rtw_dev *rtwdev,
		      struct rtw_tx_pkt_info *pkt_info,
		      struct sk_buff *skb)
{
#warning "rtw_usb_tx() undefined!"
	return 0;
}

static int rtw_usb_setup(struct rtw_dev *rtwdev)
{
#warning "rtw_usb_setup() undefined!"
	return 0;
}

static int rtw_usb_start(struct rtw_dev *rtwdev)
{
#warning "rtw_usb_start() undefined!"
	return 0;
}

static void rtw_usb_stop(struct rtw_dev *rtwdev)
{
#warning "rtw_usb_stop() undefined!"
}

static int rtw_usb_write_data_rsvd_page(struct rtw_dev *rtwdev, u8 *buf,
					u32 size)
{
#warning "rtw_usb_write_data_rsvd_page() undefined!"
	return 0;
}

static int rtw_usb_write_data_h2c(struct rtw_dev *rtwdev, u8 *buf, u32 size)
{
#warning "rtw_usb_write_data_h2c() undefined!"
	return 0;
}

static struct rtw_hci_ops rtw_usb_ops = {
	.tx = rtw_usb_tx,
	.setup = rtw_usb_setup,
	.start = rtw_usb_start,
	.stop = rtw_usb_stop,

	.read8 = rtw_usb_read8,
	.read16 = rtw_usb_read16,
	.read32 = rtw_usb_read32,
	.write8 = rtw_usb_write8,
	.write16 = rtw_usb_write16,
	.write32 = rtw_usb_write32,
	.write_data_rsvd_page = rtw_usb_write_data_rsvd_page,
	.write_data_h2c = rtw_usb_write_data_h2c,
};

static int rtw_usb_set_endpoints(struct usb_interface *intf,
				 struct rtw_dev *rtwdev)
{
	struct usb_host_interface *intf_desc = intf->cur_altsetting;
	struct usb_endpoint_descriptor *ep_desc;
	int i, out_ep = 0;

	for (i = 0; i < intf_desc->desc.bNumEndpoints; i++) {
		ep_desc = &intf_desc->endpoint[i].desc;

		if (usb_endpoint_is_bulk_out(ep_desc))
			out_ep++;
	}

	if (out_ep < _RTK_EP_OUT_MIN || out_ep >  _RTK_EP_OUT_MAX)
		return -EINVAL;

	rtwdev->hci.bulkout_num = out_ep;

	return 0;
}

static int rtw_usb_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct usb_device *udev;
	struct ieee80211_hw *hw;
	struct rtw_dev *rtwdev;
	struct rtw_usb *rtwusb;
	int drv_data_size;
	int ret = -ENODEV;

	udev = interface_to_usbdev(intf);

	drv_data_size = sizeof(struct rtw_dev) + sizeof(struct rtw_usb);
	hw = ieee80211_alloc_hw(drv_data_size, &rtw_ops);
	if (!hw) {
		dev_err(&udev->dev, "failed to allocate hw\n");
		return -ENOMEM;
	}

	rtwdev = hw->priv;
	rtwdev->hw = hw;
	rtwdev->dev = &udev->dev;
	rtwdev->chip = (struct rtw_chip_info *)id->driver_info;
	rtwdev->hci.ops = &rtw_usb_ops;
	rtwdev->hci.type = RTW_HCI_TYPE_USB;

	ret = rtw_usb_set_endpoints(intf, rtwdev);
	if (ret)
		goto err_release_hw;

	dev_info(&udev->dev, "rtw_usb_probe(): try probe %04x:%04x bulkout_num %d",
		 (int) le16_to_cpu(udev->descriptor.idVendor),
		 (int) le16_to_cpu(udev->descriptor.idProduct),
		 rtwdev->hci.bulkout_num);

	rtwusb = (struct rtw_usb *) rtwdev->priv;
	rtwusb->udev = interface_to_usbdev(intf);
	rtwusb->usb_data = kcalloc(RTL_USB_MAX_RX_COUNT, sizeof(u32),
				   GFP_KERNEL);
	if (!rtwusb->usb_data) {
		ret =  -ENODEV;
		goto err_release_hw;
	}

	/* this spin lock must be initialized early */
	spin_lock_init(&rtwusb->usb_lock);

	return ret;

err_release_hw:
	ieee80211_free_hw(hw);

	return ret;

}

static void rtw_usb_disconnect(struct usb_interface *intf)
{
	struct ieee80211_hw *hw = usb_get_intfdata(intf);
	struct rtw_dev *rtwdev;
	struct rtw_usb *rtwusb;

	if (!hw)
		return;

	rtwdev = hw->priv;
	rtwusb = (struct rtw_usb *)rtwdev->priv;
	kfree(rtwusb->usb_data);

	rtw_unregister_hw(rtwdev, hw);
	rtw_core_deinit(rtwdev);
	ieee80211_free_hw(hw);
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
