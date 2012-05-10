/*
 * Wireless USB Host Controller
 * Security support: encryption enablement, etc
 *
 * Copyright (C) 2006 Intel Corporation
 * Inaky Perez-Gonzalez <inaky.perez-gonzalez@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * FIXME: docs
 */
#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/random.h>
#include "wusbhc.h"

/*
 * DEBUG & SECURITY WARNING!!!!
 *
 * If you enable this past 1, the debug code will weaken the
 * cryptographic safety of the system (on purpose, for debugging).
 *
 * Weaken means:
 *   we print secret keys and intermediate values all the way,
 */
#undef D_LOCAL
#define D_LOCAL 2
#include <linux/uwb/debug.h>

static void wusbhc_set_gtk_callback(struct urb *urb);
static void wusbhc_gtk_rekey_done_work(struct work_struct *work);

int wusbhc_sec_create(struct wusbhc *wusbhc)
{
	wusbhc->gtk.descr.bLength = sizeof(wusbhc->gtk.descr) + sizeof(wusbhc->gtk.data);
	wusbhc->gtk.descr.bDescriptorType = USB_DT_KEY;
	wusbhc->gtk.descr.bReserved = 0;

	wusbhc->gtk_index = wusb_key_index(0, WUSB_KEY_INDEX_TYPE_GTK,
					   WUSB_KEY_INDEX_ORIGINATOR_HOST);

	INIT_WORK(&wusbhc->gtk_rekey_done_work, wusbhc_gtk_rekey_done_work);

	return 0;
}


/* Called when the HC is destroyed */
void wusbhc_sec_destroy(struct wusbhc *wusbhc)
{
}


/**
 * wusbhc_next_tkid - generate a new, currently unused, TKID
 * @wusbhc:   the WUSB host controller
 * @wusb_dev: the device whose PTK the TKID is for
 *            (or NULL for a TKID for a GTK)
 *
 * The generated TKID consist of two parts: the device's authenicated
 * address (or 0 or a GTK); and an incrementing number.  This ensures
 * that TKIDs cannot be shared between devices and by the time the
 * incrementing number wraps around the older TKIDs will no longer be
 * in use (a maximum of two keys may be active at any one time).
 */
static u32 wusbhc_next_tkid(struct wusbhc *wusbhc, struct wusb_dev *wusb_dev)
{
	u32 *tkid;
	u32 addr;

	if (wusb_dev == NULL) {
		tkid = &wusbhc->gtk_tkid;
		addr = 0;
	} else {
		tkid = &wusb_port_by_idx(wusbhc, wusb_dev->port_idx)->ptk_tkid;
		addr = wusb_dev->addr & 0x7f;
	}

	*tkid = (addr << 8) | ((*tkid + 1) & 0xff);

	return *tkid;
}

static void wusbhc_generate_gtk(struct wusbhc *wusbhc)
{
	const size_t key_size = sizeof(wusbhc->gtk.data);
	u32 tkid;

	tkid = wusbhc_next_tkid(wusbhc, NULL);

	wusbhc->gtk.descr.tTKID[0] = (tkid >>  0) & 0xff;
	wusbhc->gtk.descr.tTKID[1] = (tkid >>  8) & 0xff;
	wusbhc->gtk.descr.tTKID[2] = (tkid >> 16) & 0xff;

	get_random_bytes(wusbhc->gtk.descr.bKeyData, key_size);
}

/**
 * wusbhc_sec_start - start the security management process
 * @wusbhc: the WUSB host controller
 *
 * Generate and set an initial GTK on the host controller.
 *
 * Called when the HC is started.
 */
int wusbhc_sec_start(struct wusbhc *wusbhc)
{
	const size_t key_size = sizeof(wusbhc->gtk.data);
	int result;

	wusbhc_generate_gtk(wusbhc);

	result = wusbhc->set_gtk(wusbhc, wusbhc->gtk_tkid,
				 &wusbhc->gtk.descr.bKeyData, key_size);
	if (result < 0)
		dev_err(wusbhc->dev, "cannot set GTK for the host: %d\n",
			result);

	return result;
}

/**
 * wusbhc_sec_stop - stop the security management process
 * @wusbhc: the WUSB host controller
 *
 * Wait for any pending GTK rekeys to stop.
 */
void wusbhc_sec_stop(struct wusbhc *wusbhc)
{
	cancel_work_sync(&wusbhc->gtk_rekey_done_work);
}


/** @returns encryption type name */
const char *wusb_et_name(u8 x)
{
	switch (x) {
	case USB_ENC_TYPE_UNSECURE:	return "unsecure";
	case USB_ENC_TYPE_WIRED:	return "wired";
	case USB_ENC_TYPE_CCM_1:	return "CCM-1";
	case USB_ENC_TYPE_RSA_1:	return "RSA-1";
	default: 			return "unknown";
	}
}
EXPORT_SYMBOL_GPL(wusb_et_name);

/*
 * Set the device encryption method
 *
 * We tell the device which encryption method to use; we do this when
 * setting up the device's security.
 */
static int wusb_dev_set_encryption(struct usb_device *usb_dev, int value)
{
	int result;
	struct device *dev = &usb_dev->dev;
	struct wusb_dev *wusb_dev = usb_dev->wusb_dev;

	if (value) {
		value = wusb_dev->ccm1_etd.bEncryptionValue;
	} else {
		/* FIXME: should be wusb_dev->etd[UNSECURE].bEncryptionValue */
		value = 0;
	}
	/* Set device's */
	result = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
			USB_REQ_SET_ENCRYPTION,
			USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
			value, 0, NULL, 0, 1000 /* FIXME: arbitrary */);
	if (result < 0)
		dev_err(dev, "Can't set device's WUSB encryption to "
			"%s (value %d): %d\n",
			wusb_et_name(wusb_dev->ccm1_etd.bEncryptionType),
			wusb_dev->ccm1_etd.bEncryptionValue,  result);
	return result;
}

/*
 * Set the GTK to be used by a device.
 *
 * The device must be authenticated.
 */
static int wusb_dev_set_gtk(struct wusbhc *wusbhc, struct wusb_dev *wusb_dev)
{
	struct usb_device *usb_dev = wusb_dev->usb_dev;

	return usb_control_msg(
		usb_dev, usb_sndctrlpipe(usb_dev, 0),
		USB_REQ_SET_DESCRIPTOR,
		USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
		USB_DT_KEY << 8 | wusbhc->gtk_index, 0,
		&wusbhc->gtk.descr, wusbhc->gtk.descr.bLength,
		1000);
}


/* FIXME: prototype for adding security */
int wusb_dev_sec_add(struct wusbhc *wusbhc,
		     struct usb_device *usb_dev, struct wusb_dev *wusb_dev)
{
	int result, bytes, secd_size;
	struct device *dev = &usb_dev->dev;
	struct usb_security_descriptor secd;
	const struct usb_encryption_descriptor *etd, *ccm1_etd = NULL;
	void *secd_buf;
	const void *itr, *top;
	char buf[64];

	d_fnstart(3, dev, "(usb_dev %p, wusb_dev %p)\n", usb_dev, wusb_dev);
	result = usb_get_descriptor(usb_dev, USB_DT_SECURITY,
				    0, &secd, sizeof(secd));
	if (result < sizeof(secd)) {
		dev_err(dev, "Can't read security descriptor or "
			"not enough data: %d\n", result);
		goto error_secd;
	}
	secd_size = le16_to_cpu(secd.wTotalLength);
	d_printf(5, dev, "got %d bytes of sec descriptor, total is %d\n",
		 result, secd_size);
	secd_buf = kmalloc(secd_size, GFP_KERNEL);
	if (secd_buf == NULL) {
		dev_err(dev, "Can't allocate space for security descriptors\n");
		goto error_secd_alloc;
	}
	result = usb_get_descriptor(usb_dev, USB_DT_SECURITY,
				    0, secd_buf, secd_size);
	if (result < secd_size) {
		dev_err(dev, "Can't read security descriptor or "
			"not enough data: %d\n", result);
		goto error_secd_all;
	}
	d_printf(5, dev, "got %d bytes of sec descriptors\n", result);
	bytes = 0;
	itr = secd_buf + sizeof(secd);
	top = secd_buf + result;
	while (itr < top) {
		etd = itr;
		if (top - itr < sizeof(*etd)) {
			dev_err(dev, "BUG: bad device security descriptor; "
				"not enough data (%zu vs %zu bytes left)\n",
				top - itr, sizeof(*etd));
			break;
		}
		if (etd->bLength < sizeof(*etd)) {
			dev_err(dev, "BUG: bad device encryption descriptor; "
				"descriptor is too short "
				"(%u vs %zu needed)\n",
				etd->bLength, sizeof(*etd));
			break;
		}
		itr += etd->bLength;
		bytes += snprintf(buf + bytes, sizeof(buf) - bytes,
				  "%s (0x%02x/%02x) ",
				  wusb_et_name(etd->bEncryptionType),
				  etd->bEncryptionValue, etd->bAuthKeyIndex);
		if (etd->bEncryptionType == USB_ENC_TYPE_CCM_1)
			ccm1_etd = etd;
	}
	/* This code only supports CCM1 as of now. */
	/* FIXME: user has to choose which sec mode to use?
	 * In theory we want CCM */
	if (ccm1_etd == NULL) {
		dev_err(dev, "WUSB device doesn't support CCM1 encryption, "
			"can't use!\n");
		result = -EINVAL;
		goto error_no_ccm1;
	}
	wusb_dev->ccm1_etd = *ccm1_etd;
	dev_info(dev, "supported encryption: %s; using %s (0x%02x/%02x)\n",
		 buf, wusb_et_name(ccm1_etd->bEncryptionType),
		 ccm1_etd->bEncryptionValue, ccm1_etd->bAuthKeyIndex);
	result = 0;
	kfree(secd_buf);
out:
	d_fnend(3, dev, "(usb_dev %p, wusb_dev %p) = %d\n",
		usb_dev, wusb_dev, result);
	return result;


error_no_ccm1:
error_secd_all:
	kfree(secd_buf);
error_secd_alloc:
error_secd:
	goto out;
}

void wusb_dev_sec_rm(struct wusb_dev *wusb_dev)
{
	/* Nothing so far */
}

static void hs_printk(unsigned level, struct device *dev,
		      struct usb_handshake *hs)
{
	d_printf(level, dev,
		 "  bMessageNumber: %u\n"
		 "  bStatus:        %u\n"
		 "  tTKID:          %02x %02x %02x\n"
		 "  CDID:           %02x %02x %02x %02x %02x %02x %02x %02x\n"
		 "                  %02x %02x %02x %02x %02x %02x %02x %02x\n"
		 "  nonce:          %02x %02x %02x %02x %02x %02x %02x %02x\n"
		 "                  %02x %02x %02x %02x %02x %02x %02x %02x\n"
		 "  MIC:            %02x %02x %02x %02x %02x %02x %02x %02x\n",
		 hs->bMessageNumber, hs->bStatus,
		 hs->tTKID[2], hs->tTKID[1], hs->tTKID[0],
		 hs->CDID[0], hs->CDID[1], hs->CDID[2], hs->CDID[3],
		 hs->CDID[4], hs->CDID[5], hs->CDID[6], hs->CDID[7],
		 hs->CDID[8], hs->CDID[9], hs->CDID[10], hs->CDID[11],
		 hs->CDID[12], hs->CDID[13], hs->CDID[14], hs->CDID[15],
		 hs->nonce[0], hs->nonce[1], hs->nonce[2], hs->nonce[3],
		 hs->nonce[4], hs->nonce[5], hs->nonce[6], hs->nonce[7],
		 hs->nonce[8], hs->nonce[9], hs->nonce[10], hs->nonce[11],
		 hs->nonce[12], hs->nonce[13], hs->nonce[14], hs->nonce[15],
		 hs->MIC[0], hs->MIC[1], hs->MIC[2], hs->MIC[3],
		 hs->MIC[4], hs->MIC[5], hs->MIC[6], hs->MIC[7]);
}

/**
 * Update the address of an unauthenticated WUSB device
 *
 * Once we have successfully authenticated, we take it to addr0 state
 * and then to a normal address.
 *
 * Before the device's address (as known by it) was usb_dev->devnum |
 * 0x80 (unauthenticated address). With this we update it to usb_dev->devnum.
 */
static int wusb_dev_update_address(struct wusbhc *wusbhc,
				   struct wusb_dev *wusb_dev)
{
	int result = -ENOMEM;
	struct usb_device *usb_dev = wusb_dev->usb_dev;
	struct device *dev = &usb_dev->dev;
	u8 new_address = wusb_dev->addr & 0x7F;

	/* Set address 0 */
	result = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
				 USB_REQ_SET_ADDRESS, 0,
				 0, 0, NULL, 0, 1000 /* FIXME: arbitrary */);
	if (result < 0) {
		dev_err(dev, "auth failed: can't set address 0: %d\n",
			result);
		goto error_addr0;
	}
	result = wusb_set_dev_addr(wusbhc, wusb_dev, 0);
	if (result < 0)
		goto error_addr0;
	usb_ep0_reinit(usb_dev);

	/* Set new (authenticated) address. */
	result = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
				 USB_REQ_SET_ADDRESS, 0,
				 new_address, 0, NULL, 0,
				 1000 /* FIXME: arbitrary */);
	if (result < 0) {
		dev_err(dev, "auth failed: can't set address %u: %d\n",
			new_address, result);
		goto error_addr;
	}
	result = wusb_set_dev_addr(wusbhc, wusb_dev, new_address);
	if (result < 0)
		goto error_addr;
	usb_ep0_reinit(usb_dev);
	usb_dev->authenticated = 1;
error_addr:
error_addr0:
	return result;
}

/*
 *
 *
 */
/* FIXME: split and cleanup */
int wusb_dev_4way_handshake(struct wusbhc *wusbhc, struct wusb_dev *wusb_dev,
			    struct wusb_ckhdid *ck)
{
	int result = -ENOMEM;
	struct usb_device *usb_dev = wusb_dev->usb_dev;
	struct device *dev = &usb_dev->dev;
	u32 tkid;
	__le32 tkid_le;
	struct usb_handshake *hs;
	struct aes_ccm_nonce ccm_n;
	u8 mic[8];
	struct wusb_keydvt_in keydvt_in;
	struct wusb_keydvt_out keydvt_out;

	hs = kzalloc(3*sizeof(hs[0]), GFP_KERNEL);
	if (hs == NULL) {
		dev_err(dev, "can't allocate handshake data\n");
		goto error_kzalloc;
	}

	/* We need to turn encryption before beginning the 4way
	 * hshake (WUSB1.0[.3.2.2]) */
	result = wusb_dev_set_encryption(usb_dev, 1);
	if (result < 0)
		goto error_dev_set_encryption;

	tkid = wusbhc_next_tkid(wusbhc, wusb_dev);
	tkid_le = cpu_to_le32(tkid);

	hs[0].bMessageNumber = 1;
	hs[0].bStatus = 0;
	memcpy(hs[0].tTKID, &tkid_le, sizeof(hs[0].tTKID));
	hs[0].bReserved = 0;
	memcpy(hs[0].CDID, &wusb_dev->cdid, sizeof(hs[0].CDID));
	get_random_bytes(&hs[0].nonce, sizeof(hs[0].nonce));
	memset(hs[0].MIC, 0, sizeof(hs[0].MIC));	/* Per WUSB1.0[T7-22] */

	d_printf(1, dev, "I: sending hs1:\n");
	hs_printk(2, dev, &hs[0]);

	result = usb_control_msg(
		usb_dev, usb_sndctrlpipe(usb_dev, 0),
		USB_REQ_SET_HANDSHAKE,
		USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
		1, 0, &hs[0], sizeof(hs[0]), 1000 /* FIXME: arbitrary */);
	if (result < 0) {
		dev_err(dev, "Handshake1: request failed: %d\n", result);
		goto error_hs1;
	}

	/* Handshake 2, from the device -- need to verify fields */
	result = usb_control_msg(
		usb_dev, usb_rcvctrlpipe(usb_dev, 0),
		USB_REQ_GET_HANDSHAKE,
		USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
		2, 0, &hs[1], sizeof(hs[1]), 1000 /* FIXME: arbitrary */);
	if (result < 0) {
		dev_err(dev, "Handshake2: request failed: %d\n", result);
		goto error_hs2;
	}
	d_printf(1, dev, "got HS2:\n");
	hs_printk(2, dev, &hs[1]);

	result = -EINVAL;
	if (hs[1].bMessageNumber != 2) {
		dev_err(dev, "Handshake2 failed: bad message number %u\n",
			hs[1].bMessageNumber);
		goto error_hs2;
	}
	if (hs[1].bStatus != 0) {
		dev_err(dev, "Handshake2 failed: bad status %u\n",
			hs[1].bStatus);
		goto error_hs2;
	}
	if (memcmp(hs[0].tTKID, hs[1].tTKID, sizeof(hs[0].tTKID))) {
		dev_err(dev, "Handshake2 failed: TKID mismatch "
			"(#1 0x%02x%02x%02x vs #2 0x%02x%02x%02x)\n",
			hs[0].tTKID[0], hs[0].tTKID[1], hs[0].tTKID[2],
			hs[1].tTKID[0], hs[1].tTKID[1], hs[1].tTKID[2]);
		goto error_hs2;
	}
	if (memcmp(hs[0].CDID, hs[1].CDID, sizeof(hs[0].CDID))) {
		dev_err(dev, "Handshake2 failed: CDID mismatch\n");
		goto error_hs2;
	}

	/* Setup the CCM nonce */
	memset(&ccm_n.sfn, 0, sizeof(ccm_n.sfn));	/* Per WUSB1.0[6.5.2] */
	memcpy(ccm_n.tkid, &tkid_le, sizeof(ccm_n.tkid));
	ccm_n.src_addr = wusbhc->uwb_rc->uwb_dev.dev_addr;
	ccm_n.dest_addr.data[0] = wusb_dev->addr;
	ccm_n.dest_addr.data[1] = 0;

	/* Derive the KCK and PTK from CK, the CCM, H and D nonces */
	memcpy(keydvt_in.hnonce, hs[0].nonce, sizeof(keydvt_in.hnonce));
	memcpy(keydvt_in.dnonce, hs[1].nonce, sizeof(keydvt_in.dnonce));
	result = wusb_key_derive(&keydvt_out, ck->data, &ccm_n, &keydvt_in);
	if (result < 0) {
		dev_err(dev, "Handshake2 failed: cannot derive keys: %d\n",
			result);
		goto error_hs2;
	}
	d_printf(2, dev, "KCK:\n");
	d_dump(2, dev, keydvt_out.kck, sizeof(keydvt_out.kck));
	d_printf(2, dev, "PTK:\n");
	d_dump(2, dev, keydvt_out.ptk, sizeof(keydvt_out.ptk));

	/* Compute MIC and verify it */
	result = wusb_oob_mic(mic, keydvt_out.kck, &ccm_n, &hs[1]);
	if (result < 0) {
		dev_err(dev, "Handshake2 failed: cannot compute MIC: %d\n",
			result);
		goto error_hs2;
	}

	d_printf(2, dev, "MIC:\n");
	d_dump(2, dev, mic, sizeof(mic));
	if (memcmp(hs[1].MIC, mic, sizeof(hs[1].MIC))) {
		dev_err(dev, "Handshake2 failed: MIC mismatch\n");
		goto error_hs2;
	}

	/* Send Handshake3 */
	hs[2].bMessageNumber = 3;
	hs[2].bStatus = 0;
	memcpy(hs[2].tTKID, &tkid_le, sizeof(hs[2].tTKID));
	hs[2].bReserved = 0;
	memcpy(hs[2].CDID, &wusb_dev->cdid, sizeof(hs[2].CDID));
	memcpy(hs[2].nonce, hs[0].nonce, sizeof(hs[2].nonce));
	result = wusb_oob_mic(hs[2].MIC, keydvt_out.kck, &ccm_n, &hs[2]);
	if (result < 0) {
		dev_err(dev, "Handshake3 failed: cannot compute MIC: %d\n",
			result);
		goto error_hs2;
	}

	d_printf(1, dev, "I: sending hs3:\n");
	hs_printk(2, dev, &hs[2]);

	result = usb_control_msg(
		usb_dev, usb_sndctrlpipe(usb_dev, 0),
		USB_REQ_SET_HANDSHAKE,
		USB_DIR_OUT | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
		3, 0, &hs[2], sizeof(hs[2]), 1000 /* FIXME: arbitrary */);
	if (result < 0) {
		dev_err(dev, "Handshake3: request failed: %d\n", result);
		goto error_hs3;
	}

	d_printf(1, dev, "I: turning on encryption on host for device\n");
	d_dump(2, dev, keydvt_out.ptk, sizeof(keydvt_out.ptk));
	result = wusbhc->set_ptk(wusbhc, wusb_dev->port_idx, tkid,
				 keydvt_out.ptk, sizeof(keydvt_out.ptk));
	if (result < 0)
		goto error_wusbhc_set_ptk;

	d_printf(1, dev, "I: setting a GTK\n");
	result = wusb_dev_set_gtk(wusbhc, wusb_dev);
	if (result < 0) {
		dev_err(dev, "Set GTK for device: request failed: %d\n",
			result);
		goto error_wusbhc_set_gtk;
	}

	/* Update the device's address from unauth to auth */
	if (usb_dev->authenticated == 0) {
		d_printf(1, dev, "I: updating addres to auth from non-auth\n");
		result = wusb_dev_update_address(wusbhc, wusb_dev);
		if (result < 0)
			goto error_dev_update_address;
	}
	result = 0;
	d_printf(1, dev, "I: 4way handshke done, device authenticated\n");

error_dev_update_address:
error_wusbhc_set_gtk:
error_wusbhc_set_ptk:
error_hs3:
error_hs2:
error_hs1:
	memset(hs, 0, 3*sizeof(hs[0]));
	memset(&keydvt_out, 0, sizeof(keydvt_out));
	memset(&keydvt_in, 0, sizeof(keydvt_in));
	memset(&ccm_n, 0, sizeof(ccm_n));
	memset(mic, 0, sizeof(mic));
	if (result < 0) {
		/* error path */
		wusb_dev_set_encryption(usb_dev, 0);
	}
error_dev_set_encryption:
	kfree(hs);
error_kzalloc:
	return result;
}

/*
 * Once all connected and authenticated devices have received the new
 * GTK, switch the host to using it.
 */
static void wusbhc_gtk_rekey_done_work(struct work_struct *work)
{
	struct wusbhc *wusbhc = container_of(work, struct wusbhc, gtk_rekey_done_work);
	size_t key_size = sizeof(wusbhc->gtk.data);

	mutex_lock(&wusbhc->mutex);

	if (--wusbhc->pending_set_gtks == 0)
		wusbhc->set_gtk(wusbhc, wusbhc->gtk_tkid, &wusbhc->gtk.descr.bKeyData, key_size);

	mutex_unlock(&wusbhc->mutex);
}

static void wusbhc_set_gtk_callback(struct urb *urb)
{
	struct wusbhc *wusbhc = urb->context;

	queue_work(wusbd, &wusbhc->gtk_rekey_done_work);
}

/**
 * wusbhc_gtk_rekey - generate and distribute a new GTK
 * @wusbhc: the WUSB host controller
 *
 * Generate a new GTK and distribute it to all connected and
 * authenticated devices.  When all devices have the new GTK, the host
 * starts using it.
 *
 * This must be called after every device disconnect (see [WUSB]
 * section 6.2.11.2).
 */
void wusbhc_gtk_rekey(struct wusbhc *wusbhc)
{
	static const size_t key_size = sizeof(wusbhc->gtk.data);
	int p;

	wusbhc_generate_gtk(wusbhc);

	for (p = 0; p < wusbhc->ports_max; p++) {
		struct wusb_dev *wusb_dev;

		wusb_dev = wusbhc->port[p].wusb_dev;
		if (!wusb_dev || !wusb_dev->usb_dev | !wusb_dev->usb_dev->authenticated)
			continue;

		usb_fill_control_urb(wusb_dev->set_gtk_urb, wusb_dev->usb_dev,
				     usb_sndctrlpipe(wusb_dev->usb_dev, 0),
				     (void *)wusb_dev->set_gtk_req,
				     &wusbhc->gtk.descr, wusbhc->gtk.descr.bLength,
				     wusbhc_set_gtk_callback, wusbhc);
		if (usb_submit_urb(wusb_dev->set_gtk_urb, GFP_KERNEL) == 0)
			wusbhc->pending_set_gtks++;
	}
	if (wusbhc->pending_set_gtks == 0)
		wusbhc->set_gtk(wusbhc, wusbhc->gtk_tkid, &wusbhc->gtk.descr.bKeyData, key_size);
}
