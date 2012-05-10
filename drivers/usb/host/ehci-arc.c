/*
 * (C) Copyright David Brownell 2000-2002
 * Copyright (c) 2005 MontaVista Software
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Ported to 834x by Randy Vinson <rvinson@mvista.com> using code provided
 * by Hunter Wu.
 */

#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/usb/otg.h>
#include <mach/regs-pinctrl.h>
#include <mach/regs-power.h>
#include <mach/regs-usbctrl.h>

#include "ehci-fsl.h"

/* This parameter causes the TX FIFO buffer thresh to be increased,
 * and enables our LOCAL_5V_ON GPIO modifications.
 */
#define CHUMBY_USB_MODIFICATIONS

extern struct resource *otg_get_resources(void);

static int regs_remapped /* = 0 */;
#undef EHCI_PROC_PTC
#ifdef EHCI_PROC_PTC		/* /proc PORTSC:PTC support */
/*
 * write a PORTSC:PTC value to /proc/driver/ehci-ptc
 * to put the controller into test mode.
 */
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#define EFPSL 3			/* ehci fsl proc string length */

static int ehci_fsl_proc_read(char *page, char **start, off_t off, int count,
			      int *eof, void *data)
{
	return 0;
}

static int ehci_fsl_proc_write(struct file *file, const char __user *buffer,
			       unsigned long count, void *data)
{
	int ptc;
	u32 portsc;
	struct ehci_hcd *ehci = (struct ehci_hcd *) data;
	char str[EFPSL] = {0};

	if (count > EFPSL-1)
		return -EINVAL;

	if (copy_from_user(str, buffer, count))
		return -EFAULT;

	str[count] = '\0';

	ptc = simple_strtoul(str, NULL, 0);

	portsc = ehci_readl(ehci, &ehci->regs->port_status[0]);
	portsc &= ~(0xf << 16);
	portsc |= (ptc << 16);
	printk(KERN_INFO "PTC %x  portsc %08x\n", ptc, portsc);

	ehci_writel(ehci, portsc, &ehci->regs->port_status[0]);

	return count;
}

static int ehci_testmode_init(struct ehci_hcd *ehci)
{
	struct proc_dir_entry *entry;

	entry = create_proc_read_entry("driver/ehci-ptc", 0644, NULL,
				       ehci_fsl_proc_read, ehci);
	if (!entry)
		return -ENODEV;

	entry->write_proc = ehci_fsl_proc_write;
	return 0;
}
#else
static int ehci_testmode_init(struct ehci_hcd *ehci)
{
	return 0;
}
#endif	/* /proc PORTSC:PTC support */


/* PCI-based HCs are common, but plenty of non-PCI HCs are used too */

/* configure so an HC device and id are always provided */
/* always called with process context; sleeping is OK */

/**
 * usb_hcd_fsl_probe - initialize FSL-based HCDs
 * @drvier: Driver to be used for this HCD
 * @pdev: USB Host Controller being probed
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller.
 *
 */
static int usb_hcd_fsl_probe(const struct hc_driver *driver,
			     struct platform_device *pdev)
{
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;
	struct usb_hcd *hcd;
	struct resource *res;
	int irq;
	int retval;
	u32 temp;

	pr_debug("initializing FSL-SOC USB Controller\n");

	/* Need platform data for setup */
	if (!pdata) {
		dev_err(&pdev->dev,
			"No platform data for %s.\n", pdev->dev.bus_id);
		return -ENODEV;
	}

	/*
	 * This is a host mode driver, verify that we're supposed to be
	 * in host mode.
	 */
	if (!((pdata->operating_mode == FSL_USB2_DR_HOST) ||
	      (pdata->operating_mode == FSL_USB2_MPH_HOST) ||
	      (pdata->operating_mode == FSL_USB2_DR_OTG))) {
		dev_err(&pdev->dev,
			"Non Host Mode configured for %s. "
			"Wrong driver linked.\n", pdev->dev.bus_id);
		return -ENODEV;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, pdev->dev.bus_id);
	if (!hcd) {
		retval = -ENOMEM;
		goto err1;
	}

#if defined(CONFIG_USB_OTG)
	if (pdata->operating_mode == FSL_USB2_DR_OTG) {
		res = otg_get_resources();
		if (!res) {
			dev_err(&pdev->dev,
				"Found HC with no IRQ. Check %s setup!\n",
				pdev->dev.bus_id);
			return -ENODEV;
		}
		irq = res[1].start;
		hcd->rsrc_start = res[0].start;
		hcd->rsrc_len = res[0].end - res[0].start + 1;
	} else
#endif
	{
		if ((pdev->dev.parent) &&
			(to_platform_device(pdev->dev.parent)->resource)) {
			pdev->resource =
				to_platform_device(pdev->dev.parent)->resource;
			pdev->num_resources =
			to_platform_device(pdev->dev.parent)->num_resources;
		}

		res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (!res) {
			dev_err(&pdev->dev,
				"Found HC with no IRQ. Check %s setup!\n",
				pdev->dev.bus_id);
			return -ENODEV;
		}
		irq = res->start;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		hcd->rsrc_start = res->start;
		hcd->rsrc_len = resource_size(res);

		if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len,
					driver->description)) {
			dev_dbg(&pdev->dev, "controller already in use\n");
			retval = -EBUSY;
			goto err2;
		}
	}

	if (!(pdata->port_enables & FSL_USB2_DONT_REMAP)) {
		hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);

		if (hcd->regs == NULL) {
			dev_dbg(&pdev->dev, "error mapping memory\n");
			retval = -EFAULT;
			goto err3;
		}
		regs_remapped = 1;
	} else
		hcd->regs = (void __iomem *)(u32)(hcd->rsrc_start);

	pdata->regs = hcd->regs;

	/*
	 * do platform specific init: check the clock, grab/config pins, etc.
	 */
	if (pdata->platform_init && pdata->platform_init(pdev)) {
		retval = -ENODEV;
		goto err3;
	}

	fsl_platform_set_host_mode(hcd);
	hcd->power_budget = pdata->power_budget;

	retval = usb_add_hcd(hcd, irq, IRQF_DISABLED | IRQF_SHARED);
	if (retval != 0)
		goto err4;

	fsl_platform_set_vbus_power(pdata, 1);

#ifdef CONFIG_USB_OTG
	if (pdata->operating_mode == FSL_USB2_DR_OTG) {
		struct ehci_hcd *ehci = hcd_to_ehci(hcd);

		dbg("pdev=0x%p  hcd=0x%p  ehci=0x%p\n", pdev, hcd, ehci);

		ehci->transceiver = otg_get_transceiver();
		dbg("ehci->transceiver=0x%p\n", ehci->transceiver);

		if (ehci->transceiver) {
			retval = otg_set_host(ehci->transceiver,
					      &ehci_to_hcd(ehci)->self);
			if (retval) {
				if (ehci->transceiver)
					put_device(ehci->transceiver->dev);
				goto err4;
			}
		} else {
			printk(KERN_ERR "can't find transceiver\n");
			retval = -ENODEV;
			goto err4;
		}
	}
#endif

	fsl_platform_set_ahb_burst(hcd);
	ehci_testmode_init(hcd_to_ehci(hcd));
	return retval;

err4:
	if (regs_remapped)
		iounmap(hcd->regs);
err3:
	if (pdata->operating_mode != FSL_USB2_DR_OTG)
		release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err2:
	usb_put_hcd(hcd);
err1:
	dev_err(&pdev->dev, "init %s fail, %d\n", pdev->dev.bus_id, retval);
	if (pdata->platform_uninit)
		pdata->platform_uninit(pdata);
	return retval;
}

/* may be called without controller electrically present */
/* may be called with controller, bus, and devices active */

/**
 * usb_hcd_fsl_remove - shutdown processing for FSL-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_fsl_probe().
 *
 */
static void usb_hcd_fsl_remove(struct usb_hcd *hcd,
			       struct platform_device *pdev)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;

	/* DDD shouldn't we turn off the power here? */
	fsl_platform_set_vbus_power(pdata, 0);

	if (ehci->transceiver) {
		(void)otg_set_host(ehci->transceiver, 0);
		put_device(ehci->transceiver->dev);
	} else {
		release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	}

	usb_remove_hcd(hcd);
	usb_put_hcd(hcd);

	/*
	 * do platform specific un-initialization:
	 * release iomux pins, etc.
	 */
	if (pdata->platform_uninit)
		pdata->platform_uninit(pdata);

	if (regs_remapped)
		iounmap(hcd->regs);
}

static void fsl_setup_phy(struct ehci_hcd *ehci,
			  enum fsl_usb2_phy_modes phy_mode, int port_offset)
{
	u32 portsc;

	portsc = ehci_readl(ehci, &ehci->regs->port_status[port_offset]);
	portsc &= ~(PORT_PTS_MSK | PORT_PTS_PTW);

	switch (phy_mode) {
	case FSL_USB2_PHY_ULPI:
		portsc |= PORT_PTS_ULPI;
		break;
	case FSL_USB2_PHY_SERIAL:
		portsc |= PORT_PTS_SERIAL;
		break;
	case FSL_USB2_PHY_UTMI_WIDE:
		portsc |= PORT_PTS_PTW;
		/* fall through */
	case FSL_USB2_PHY_UTMI:
		portsc |= PORT_PTS_UTMI;
		break;
	case FSL_USB2_PHY_NONE:
		break;
	}
	ehci_writel(ehci, portsc, &ehci->regs->port_status[port_offset]);
}

#ifdef CHUMBY_USB_MODIFICATIONS
static int ehci_fsl_run (struct usb_hcd *hcd)
{
	int ret = ehci_run(hcd);

	/* Alter the TX fifo threshhold.  If we don't do this, the buffer will
	 * transmit when it's not completely filled, which can cause problems
	 * with e.g. the rt73 driver.
	 * Note this parameter will have to be tuned!
	 */
	HW_USBCTRL_TXFILLTUNING_WR(0x003f0020);

	return ret;
}
#endif

/* called after powerup, by probe or system-pm "wakeup" */
static int ehci_fsl_reinit(struct ehci_hcd *ehci)
{
	fsl_platform_usb_setup(ehci);
	ehci_port_power(ehci, 0);
	return 0;
}

/* called during probe() after chip reset completes */
static int ehci_fsl_setup(struct usb_hcd *hcd)
{
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	int retval;
	struct fsl_usb2_platform_data *pdata;
	pdata = hcd->self.controller->platform_data;

	ehci->big_endian_desc = pdata->big_endian_desc;
	ehci->big_endian_mmio = pdata->big_endian_mmio;

	/* EHCI registers start at offset 0x100 */
	ehci->caps = hcd->regs + 0x100;
	ehci->regs = hcd->regs + 0x100 +
	    HC_LENGTH(ehci_readl(ehci, &ehci->caps->hc_capbase));
	dbg_hcs_params(ehci, "reset");
	dbg_hcc_params(ehci, "reset");

	/* cache this readonly data; minimize chip reads */
	ehci->hcs_params = ehci_readl(ehci, &ehci->caps->hcs_params);

	retval = ehci_halt(ehci);

	/* data structure init */
	retval = ehci_init(hcd);
	if (retval)
		return retval;

	hcd->has_tt = 1;

	ehci->sbrn = 0x20;

	ehci_reset(ehci);

	retval = ehci_fsl_reinit(ehci);


#ifdef CHUMBY_USB_MODIFICATIONS
	/* Also, disable the IRQ for the 5V line.  Turning on the screen causes
	 * a massive drop in power, and the battery circuitry thinks we've
	 * pulled the 5V line.
	 */
	HW_POWER_CTRL_CLR(1);




	/* Wait just enough time for the brownout.  Any less than this and it
	 * reboots when we bring the USB card up, or segfaults.
	 */
	msleep(5);


	/* Set bank 0, pins 26 and 28 to GPIO. */
	HW_PINCTRL_MUXSEL1_SET(0x03300000);


	/* Set both pins to low, which ensures a reset takes place. */
	HW_PINCTRL_DOUT0_CLR(0x24000000);
	HW_PINCTRL_DOE0_SET(0x24000000);
	HW_PINCTRL_DOUT0_CLR(0x24000000);
	msleep(5);


	/* Delay for one second, giving most USB devices the chance to
	 * recognize they've been "unplugged".
	 */
	msleep(1000);


	/* Write a "1" to bank 0, pin 28. */
	HW_PINCTRL_DOUT0_SET(0x20000000);
	msleep(5);


	/* Write a "1" to bank 0, pin 26, to disable USB.  This ensures a reset.
	 * Wait a short time after, then re-enable the IRQ that fires when power
	 * is pulled.  This is because by now, the voltages should have
	 * stabilized.
	 */
	HW_PINCTRL_DOUT0_SET(0x04000000);
	msleep(5);


	/* Re-enable the power interrupt. */
	HW_POWER_CTRL_SET(1);


#endif /*CHUMBY_USB_MODIFICATIONS*/

	return retval;
}

static const struct hc_driver ehci_fsl_hc_driver = {
	.description = hcd_name,
	.product_desc = "Freescale On-Chip EHCI Host Controller",
	.hcd_priv_size = sizeof(struct ehci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq = ehci_irq,
	.flags = FSL_PLATFORM_HC_FLAGS,

	/*
	 * basic lifecycle operations
	 */
	.reset = ehci_fsl_setup,
#ifdef CHUMBY_USB_MODIFICATIONS
	.start = ehci_fsl_run,
#else
	.start = ehci_run,
#endif
	.stop = ehci_stop,
	.shutdown = ehci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue = ehci_urb_enqueue,
	.urb_dequeue = ehci_urb_dequeue,
	.endpoint_disable = ehci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number = ehci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data = ehci_hub_status_data,
	.hub_control = ehci_hub_control,
	.bus_suspend = ehci_bus_suspend,
	.bus_resume = ehci_bus_resume,
	.start_port_reset = ehci_start_port_reset,
};

static int ehci_fsl_drv_probe(struct platform_device *pdev)
{
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;

	if (usb_disabled())
		return -ENODEV;

	return usb_hcd_fsl_probe(&ehci_fsl_hc_driver, pdev);
}

static int ehci_fsl_drv_remove(struct platform_device *pdev)
{
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;
	struct usb_hcd *hcd = platform_get_drvdata(pdev);

	usb_hcd_fsl_remove(hcd, pdev);
	return 0;
}

static int ehci_fsl_drv_shutdown(struct platform_device *pdev)
{
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;

	usb_hcd_platform_shutdown(pdev);
}

#ifdef CONFIG_PM
/* suspend/resume, section 4.3 */

/* These routines rely on the bus (pci, platform, etc)
 * to handle powerdown and wakeup, and currently also on
 * transceivers that don't need any software attention to set up
 * the right sort of wakeup.
 *
 * They're also used for turning on/off the port when doing OTG.
 */
#if defined(CONFIG_USB_EHCI_ARC_H2_WAKE_UP) || \
	defined(CONFIG_USB_EHCI_ARC_OTG_WAKE_UP)
extern void usb_wakeup_set(struct device *wkup_dev, int para);
#endif
static int ehci_fsl_drv_suspend(struct platform_device *pdev,
				pm_message_t message)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 tmp;
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;

#ifdef DEBUG
	u32 mode = ehci_readl(ehci, hcd->regs + FSL_SOC_USB_USBMODE);
	mode &= USBMODE_CM_MASK;
	tmp = ehci_readl(ehci, hcd->regs + 0x140);	/* usbcmd */

	printk(KERN_DEBUG "%s('%s'): suspend=%d already_suspended=%d "
	       "mode=%d  usbcmd %08x\n", __func__, pdata->name,
	       pdata->suspended, pdata->already_suspended, mode, tmp);
#endif

	/*
	 * If the controller is already suspended, then this must be a
	 * PM suspend.  Remember this fact, so that we will leave the
	 * controller suspended at PM resume time.
	 */
	if (pdata->suspended) {
		pr_debug("%s: already suspended, leaving early\n", __func__);
		pdata->already_suspended = 1;
		return 0;
	}

	pr_debug("%s: suspending...\n", __func__);

	printk(KERN_INFO "USB Host suspended\n");

	hcd->state = HC_STATE_SUSPENDED;
	pdev->dev.power.power_state = PMSG_SUSPEND;

	/* ignore non-host interrupts */
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	/* stop the controller */
	tmp = ehci_readl(ehci, &ehci->regs->command);
	tmp &= ~CMD_RUN;
	ehci_writel(ehci, tmp, &ehci->regs->command);

	/* save EHCI registers */
	pdata->pm_command = ehci_readl(ehci, &ehci->regs->command);
	pdata->pm_command &= ~CMD_RUN;
	pdata->pm_status  = ehci_readl(ehci, &ehci->regs->status);
	pdata->pm_intr_enable  = ehci_readl(ehci, &ehci->regs->intr_enable);
	pdata->pm_frame_index  = ehci_readl(ehci, &ehci->regs->frame_index);
	pdata->pm_segment  = ehci_readl(ehci, &ehci->regs->segment);
	pdata->pm_frame_list  = ehci_readl(ehci, &ehci->regs->frame_list);
	pdata->pm_async_next  = ehci_readl(ehci, &ehci->regs->async_next);
	pdata->pm_configured_flag  =
		ehci_readl(ehci, &ehci->regs->configured_flag);
	pdata->pm_portsc = ehci_readl(ehci, &ehci->regs->port_status[0]);

	/* clear the W1C bits */
	pdata->pm_portsc &= cpu_to_hc32(ehci, ~PORT_RWC_BITS);

	pdata->suspended = 1;
#if defined(CONFIG_USB_EHCI_ARC_H2_WAKE_UP) || \
	defined(CONFIG_USB_EHCI_ARC_OTG_WAKE_UP)
	/* enable remote wake up irq */
	usb_wakeup_set(&(pdev->dev), 1);

	/* We CAN NOT enable wake up by connetion and disconnection
	 * concurrently */
	tmp = ehci_readl(ehci, &ehci->regs->port_status[0]);
	/* if there is no usb device connectted */
	if (tmp & PORT_CONNECT) {
		/* enable wake up by usb device disconnection */
		tmp |= PORT_WKDISC_E;
		tmp &= ~(PORT_WKOC_E | PORT_WKCONN_E);
	} else {
		/* enable wake up by usb device insertion */
		tmp |= PORT_WKCONN_E;
		tmp &= ~(PORT_WKOC_E | PORT_WKDISC_E);
	}
	ehci_writel(ehci, tmp, &ehci->regs->port_status[0]);

	/* Set the port into suspend */
	tmp = ehci_readl(ehci, &ehci->regs->port_status[0]);
	tmp |= PORT_SUSPEND;
	ehci_writel(ehci, tmp, &ehci->regs->port_status[0]);

	/* Disable PHY clock */
	tmp = ehci_readl(ehci, &ehci->regs->port_status[0]);
	tmp |= PORT_PHCD;
	ehci_writel(ehci, tmp, &ehci->regs->port_status[0]);
#else
	/* clear PP to cut power to the port */
	tmp = ehci_readl(ehci, &ehci->regs->port_status[0]);
	tmp &= ~PORT_POWER;
	ehci_writel(ehci, tmp, &ehci->regs->port_status[0]);
#endif
	return 0;
}

static int ehci_fsl_drv_resume(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	u32 tmp;
	struct fsl_usb2_platform_data *pdata = pdev->dev.platform_data;

	printk(KERN_INFO "USB Host resumed\n");

	pr_debug("%s('%s'): suspend=%d already_suspended=%d\n", __func__,
		pdata->name, pdata->suspended, pdata->already_suspended);

	/*
	 * If the controller was already suspended at suspend time,
	 * then don't resume it now.
	 */
	if (pdata->already_suspended) {
		pr_debug("already suspended, leaving early\n");
		pdata->already_suspended = 0;
		return 0;
	}

	if (!pdata->suspended) {
		pr_debug("not suspended, leaving early\n");
		return 0;
	}

	pdata->suspended = 0;

	pr_debug("%s resuming...\n", __func__);

	/* set host mode */
	fsl_platform_set_host_mode(hcd);

	/* restore EHCI registers */
	ehci_writel(ehci, pdata->pm_command, &ehci->regs->command);
	ehci_writel(ehci, pdata->pm_intr_enable, &ehci->regs->intr_enable);
	ehci_writel(ehci, pdata->pm_frame_index, &ehci->regs->frame_index);
	ehci_writel(ehci, pdata->pm_segment, &ehci->regs->segment);
	ehci_writel(ehci, pdata->pm_frame_list, &ehci->regs->frame_list);
	ehci_writel(ehci, pdata->pm_async_next, &ehci->regs->async_next);
	ehci_writel(ehci, pdata->pm_configured_flag,
		    &ehci->regs->configured_flag);
	ehci_writel(ehci, pdata->pm_portsc, &ehci->regs->port_status[0]);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	hcd->state = HC_STATE_RUNNING;
	pdev->dev.power.power_state = PMSG_ON;

	tmp = ehci_readl(ehci, &ehci->regs->command);
	tmp |= CMD_RUN;
	ehci_writel(ehci, tmp, &ehci->regs->command);

	usb_hcd_resume_root_hub(hcd);

	return 0;
}
#endif				/* CONFIG_USB_OTG */

MODULE_ALIAS("fsl-ehci");

static struct platform_driver ehci_fsl_driver = {
	.probe = ehci_fsl_drv_probe,
	.remove = ehci_fsl_drv_remove,
	.shutdown = ehci_fsl_drv_shutdown,
#ifdef CONFIG_PM
	.suspend = ehci_fsl_drv_suspend,
	.resume = ehci_fsl_drv_resume,
#endif
	.driver = {
		   .name = "fsl-ehci",
		   },
};
