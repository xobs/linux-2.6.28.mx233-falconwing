/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/errno.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/clock.h>
#include <mach/gpio.h>

#include "iomux.h"

/*!
 * @file mach-mx51/mx51_3stack_gpio.c
 *
 * @brief This file contains all the GPIO setup functions for the board.
 *
 * @ingroup GPIO
 */
#define ATA_PAD_CONFIG (PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH)

static struct mxc_iomux_pin_cfg __initdata mxc_iomux_pins[] = {
	/* CSI0 */
	{
	MX51_PIN_CSI1_D8, IOMUX_CONFIG_ALT3,
	PAD_CTL_PKE_ENABLE,
	MUX_IN_GPIO3_IPP_IND_G_IN_12_SELECT_INPUT,
	INPUT_CTL_PATH1,
	},
	{
	MX51_PIN_CSI1_D9, IOMUX_CONFIG_ALT3,
	PAD_CTL_PKE_ENABLE,
	},
	{
	 MX51_PIN_CSI1_D10, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D11, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D12, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D13, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D14, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D15, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D16, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D17, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D18, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_D19, IOMUX_CONFIG_ALT0, PAD_CTL_HYS_NONE,
	 },
	{
	 MX51_PIN_CSI1_VSYNC, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX51_PIN_CSI1_HSYNC, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_SRE_SLOW),
	 },
	{
	 MX51_PIN_EIM_A26, IOMUX_CONFIG_ALT5 | IOMUX_CONFIG_SION,
	 (PAD_CTL_SRE_SLOW | PAD_CTL_DRV_MEDIUM | PAD_CTL_100K_PU |
	 PAD_CTL_HYS_ENABLE | PAD_CTL_DRV_VOT_HIGH),
	 MUX_IN_HSC_MIPI_MIX_IPP_IND_SENS2_DATA_EN_SELECT_INPUT,
	 INPUT_CTL_PATH0,
	 },
	{			/* SPI1 */
	 MX51_PIN_CSPI1_MISO, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_CSPI1_MOSI, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_CSPI1_RDY, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_CSPI1_SCLK, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_DRV_HIGH |
				  PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_OWIRE_LINE, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE |
		 PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_DRV_HIGH |
		 PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP2_DAT15, IOMUX_CONFIG_ALT5,
	 },
	{
	 MX51_PIN_DI_GP2, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_DI_GP3, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_COL0, IOMUX_CONFIG_ALT0,
	},
	{
	 MX51_PIN_KEY_COL1, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_COL2, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_COL3, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_COL4, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_COL5, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_ROW0, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_ROW1, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_ROW2, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_KEY_ROW3, IOMUX_CONFIG_ALT0,
	 },
	{			/* AUD3_TXD */
	 MX51_PIN_AUD3_BB_TXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH |
	    PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_100K_PU |
	    PAD_CTL_HYS_NONE | PAD_CTL_DDR_INPUT_CMOS | PAD_CTL_DRV_VOT_LOW),
	 },
	{			/* AUD3_RXD */
	 MX51_PIN_AUD3_BB_RXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH |
	    PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_100K_PU |
	    PAD_CTL_HYS_NONE | PAD_CTL_DDR_INPUT_CMOS | PAD_CTL_DRV_VOT_LOW),
	 },
	{			/* AUD3_CLK */
	 MX51_PIN_AUD3_BB_CK, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH |
	    PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_100K_PU |
	    PAD_CTL_HYS_NONE | PAD_CTL_DDR_INPUT_CMOS | PAD_CTL_DRV_VOT_LOW),
	 },
	{			/* AUD3_FS */
	 MX51_PIN_AUD3_BB_FS, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH |
	    PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_100K_PU |
	    PAD_CTL_HYS_NONE | PAD_CTL_DDR_INPUT_CMOS | PAD_CTL_DRV_VOT_LOW),
	 },
	{
	 MX51_PIN_EIM_D16, IOMUX_CONFIG_ALT1,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
			  PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_EIM_A27, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{
	MX51_PIN_EIM_EB2, IOMUX_CONFIG_ALT1,
	PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL | PAD_CTL_100K_PD,
	},
	{
	 MX51_PIN_EIM_DTACK, IOMUX_CONFIG_GPIO,
	 (PAD_CTL_PKE_ENABLE | PAD_CTL_100K_PU),
	 },
	{
	 MX51_PIN_EIM_CS2, IOMUX_CONFIG_GPIO,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_HYS_NONE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_100K_PU | PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_EIM_CRE, IOMUX_CONFIG_GPIO,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_HYS_NONE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_100K_PU | PAD_CTL_PKE_ENABLE | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DI_GP4, IOMUX_CONFIG_ALT2,
	 },
	{
	 MX51_PIN_DISPB2_SER_DIN, IOMUX_CONFIG_GPIO,
	 0,
	 MUX_IN_GPIO3_IPP_IND_G_IN_5_SELECT_INPUT,
	 INPUT_CTL_PATH1,
	 },
	{
	 MX51_PIN_DISPB2_SER_RS, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX51_PIN_DISPB2_SER_DIO, IOMUX_CONFIG_GPIO,
	 },
	{ /* TO2 */
	MX51_PIN_DI1_D1_CS, IOMUX_CONFIG_ALT4,
	},
	{ /* TO2 */
	MX51_PIN_DI1_D0_CS, IOMUX_CONFIG_ALT1,
	},
	{ /* TO2 */
	MX51_PIN_DI1_PIN11, IOMUX_CONFIG_ALT1,
	},
	{ /* TO2 */
	MX51_PIN_DI1_PIN12, IOMUX_CONFIG_ALT1,
	},
	{ /* TO2 */
	MX51_PIN_DI1_PIN13, IOMUX_CONFIG_ALT1,
	},
#ifdef CONFIG_FB_MXC_CLAA_WVGA_SYNC_PANEL
	{
	 MX51_PIN_DISP1_DAT0, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT4, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT5, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT6, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT7, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT8, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT9, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT10, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT11, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT12, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT13, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT14, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT15, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT16, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT17, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT18, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT19, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT20, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT21, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT22, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_DISP1_DAT23, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_DRV_LOW | PAD_CTL_SRE_FAST),
	 },
#endif
	{
	 MX51_PIN_I2C1_CLK, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 0x1E4,
	 },
	{
	 MX51_PIN_I2C1_DAT, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 0x1E4,
	 },
	{
	 MX51_PIN_GPIO1_6, IOMUX_CONFIG_GPIO,
	 },
	{
	 MX51_PIN_GPIO1_7, IOMUX_CONFIG_ALT2,
	 (PAD_CTL_DRV_HIGH | PAD_CTL_PUE_PULL |
			 PAD_CTL_100K_PU | PAD_CTL_PKE_ENABLE |
			 PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_GPIO1_2, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION,
	 (PAD_CTL_SRE_FAST | PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_DRV_HIGH |
	  PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE),
	 MUX_IN_I2C2_IPP_SCL_IN_SELECT_INPUT, INPUT_CTL_PATH3,
	 },
	{
	 MX51_PIN_GPIO1_3, IOMUX_CONFIG_ALT2 | IOMUX_CONFIG_SION,
	 (PAD_CTL_SRE_FAST | PAD_CTL_ODE_OPENDRAIN_ENABLE | PAD_CTL_DRV_HIGH |
	  PAD_CTL_100K_PU | PAD_CTL_HYS_ENABLE),
	 MUX_IN_I2C2_IPP_SDA_IN_SELECT_INPUT, INPUT_CTL_PATH3,
	 },
	{
	 MX51_PIN_USBH1_STP, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_CLK */
	 MX51_PIN_USBH1_CLK, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS),
	 },
	{			/* USBH1_DIR */
	 MX51_PIN_USBH1_DIR, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS),
	 },
	{			/* USBH1_NXT */
	 MX51_PIN_USBH1_NXT, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE | PAD_CTL_DDR_INPUT_CMOS),
	 },
	{			/* USBH1_DATA0 */
	 MX51_PIN_USBH1_DATA0, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA1 */
	 MX51_PIN_USBH1_DATA1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA2 */
	 MX51_PIN_USBH1_DATA2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA3 */
	 MX51_PIN_USBH1_DATA3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA4 */
	 MX51_PIN_USBH1_DATA4, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA5 */
	 MX51_PIN_USBH1_DATA5, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA6 */
	 MX51_PIN_USBH1_DATA6, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	{			/* USBH1_DATA7 */
	 MX51_PIN_USBH1_DATA7, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_100K_PU |
	  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE | PAD_CTL_HYS_ENABLE),
	 },
	 {			/* USB1_OC */
	 MX51_PIN_GPIO1_9, IOMUX_CONFIG_ALT1,
	 (PAD_CTL_SRE_SLOW | PAD_CTL_DRV_LOW | PAD_CTL_ODE_OPENDRAIN_NONE |
			  PAD_CTL_PUE_KEEPER | PAD_CTL_PKE_ENABLE |
			  PAD_CTL_HYS_ENABLE),
	 },
	{			/* USB1_PWR */
	 MX51_PIN_GPIO1_8, IOMUX_CONFIG_ALT1,
	 (PAD_CTL_SRE_FAST | PAD_CTL_DRV_HIGH | PAD_CTL_ODE_OPENDRAIN_NONE |
			  PAD_CTL_PKE_NONE | PAD_CTL_HYS_ENABLE),
	 },
	{
	 MX51_PIN_SD1_CMD, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD1_CLK, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD1_DATA0, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD1_DATA1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD1_DATA2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD1_DATA3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_GPIO1_0, IOMUX_CONFIG_GPIO | IOMUX_CONFIG_SION,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_100K_PU),
	 },
	{
	 MX51_PIN_GPIO1_1, IOMUX_CONFIG_GPIO | IOMUX_CONFIG_SION,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_100K_PU),
	 },
	{
	 MX51_PIN_SD2_CMD, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD2_CLK, IOMUX_CONFIG_ALT0 | IOMUX_CONFIG_SION,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD2_DATA0, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD2_DATA1, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD2_DATA2, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_SD2_DATA3, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_DRV_MAX | PAD_CTL_22K_PU | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_UART1_RXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART1_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH0,
	 },
	{
	 MX51_PIN_UART1_TXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_UART1_RTS, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH),
	 MUX_IN_UART1_IPP_UART_RTS_B_SELECT_INPUT,
	 INPUT_CTL_PATH0,
	 },
	{
	 MX51_PIN_UART1_CTS, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH),
	 },
	{
	 MX51_PIN_UART2_RXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART2_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH2,
	 },
	{
	 MX51_PIN_UART2_TXD, IOMUX_CONFIG_ALT0,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_PULL |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_EIM_D26, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART2_IPP_UART_RTS_B_SELECT_INPUT,
	 INPUT_CTL_PATH3,
	 },
	{
	 MX51_PIN_EIM_D25, IOMUX_CONFIG_ALT4,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_UART3_RXD, IOMUX_CONFIG_ALT1,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART3_IPP_UART_RXD_MUX_SELECT_INPUT,
	 INPUT_CTL_PATH4,
	 },
	{
	 MX51_PIN_UART3_TXD, IOMUX_CONFIG_ALT1,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
	{
	 MX51_PIN_EIM_D27, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 MUX_IN_UART3_IPP_UART_RTS_B_SELECT_INPUT,
	 INPUT_CTL_PATH3,
	 },
	{
	 MX51_PIN_EIM_D24, IOMUX_CONFIG_ALT3,
	 (PAD_CTL_HYS_NONE | PAD_CTL_PKE_ENABLE | PAD_CTL_PUE_KEEPER |
	  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST),
	 },
};

static struct mxc_iomux_pin_cfg __initdata ata_iomux_pins[] = {
	{
	 MX51_PIN_NANDF_ALE, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CS2, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CS3, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_RE_B, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_WE_B, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_CLE, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_RB0, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_WP_B, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	/* TO 2.0 */
	{
	 MX51_PIN_GPIO_NAND, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	/* TO 1.0 */
	{
	 MX51_PIN_NANDF_RB5, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_RB1, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D0, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D1, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D2, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D3, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D4, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D5, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D6, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D7, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D8, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D9, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D10, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D11, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D12, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D13, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D14, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
	{
	 MX51_PIN_NANDF_D15, IOMUX_CONFIG_ALT1,
	 ATA_PAD_CONFIG,
	 },
};

static struct mxc_iomux_pin_cfg __initdata nand_iomux_pins[] = {
	{
	 MX51_PIN_NANDF_CS0, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS1, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS2, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS3, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT0,
	 },
	{
	 MX51_PIN_NANDF_CS7, IOMUX_CONFIG_ALT0,
	 },
	/* TO2 */
	{
	 MX51_PIN_GPIO_NAND, IOMUX_CONFIG_ALT0,
	 },
	/* TO1 */
	{
	 MX51_PIN_NANDF_RB5, IOMUX_CONFIG_ALT0,
	 },
};

static struct mxc_iomux_pin_cfg __initdata sim_iomux_pins[] = {
	{
	 MX51_PIN_NANDF_CS4, IOMUX_CONFIG_ALT6,
	 PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH |
	 PAD_CTL_HYS_NONE | PAD_CTL_47K_PU |
	 PAD_CTL_PUE_KEEPER | PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_PKE_ENABLE,
	 },
	{
	 MX51_PIN_NANDF_CS5, IOMUX_CONFIG_ALT6,
	 PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH |
	 PAD_CTL_HYS_NONE | PAD_CTL_47K_PU |
	 PAD_CTL_PUE_KEEPER | PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_PKE_ENABLE,
	 },
	{
	 MX51_PIN_NANDF_CS6, IOMUX_CONFIG_ALT6,
	 PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH |
	 PAD_CTL_HYS_NONE | PAD_CTL_100K_PD |
	 PAD_CTL_PUE_PULL | PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_PKE_ENABLE,
	 },
	{
	 MX51_PIN_NANDF_CS7, IOMUX_CONFIG_ALT6,
	 PAD_CTL_DRV_HIGH | PAD_CTL_DRV_VOT_HIGH |
	 PAD_CTL_HYS_NONE | PAD_CTL_22K_PU |
	 PAD_CTL_PUE_PULL | PAD_CTL_ODE_OPENDRAIN_NONE | PAD_CTL_PKE_ENABLE,
	 },
};

static int __initdata enable_ata = { 0 };
static int __init ata_setup(char *__unused)
{
	enable_ata = 1;
	return 1;
}

__setup("ata", ata_setup);

static int __initdata enable_sim = { 0 };
static int __init sim_setup(char *__unused)
{
	enable_sim = 1;
	return 1;
}

__setup("sim", sim_setup);

void __init mx51_3stack_io_init(void)
{
	int i, num;
	struct mxc_iomux_pin_cfg *pin_ptr;

	for (i = 0; i < ARRAY_SIZE(mxc_iomux_pins); i++) {
		mxc_request_iomux(mxc_iomux_pins[i].pin,
				  mxc_iomux_pins[i].mux_mode);
		if (mxc_iomux_pins[i].pad_cfg)
			mxc_iomux_set_pad(mxc_iomux_pins[i].pin,
					  mxc_iomux_pins[i].pad_cfg);
		if (mxc_iomux_pins[i].in_select)
			mxc_iomux_set_input(mxc_iomux_pins[i].in_select,
					  mxc_iomux_pins[i].in_mode);
	}

	if (enable_ata) {
		pin_ptr = ata_iomux_pins;
		num = ARRAY_SIZE(ata_iomux_pins);
	} else if (enable_sim) {
		pin_ptr = sim_iomux_pins;
		num = ARRAY_SIZE(sim_iomux_pins);
	} else {
		pin_ptr = nand_iomux_pins;
		num = ARRAY_SIZE(nand_iomux_pins);
	}

	for (i = 0; i < num; i++) {
		mxc_request_iomux(pin_ptr[i].pin, pin_ptr[i].mux_mode);
		if (pin_ptr[i].pad_cfg)
			mxc_iomux_set_pad(pin_ptr[i].pin, pin_ptr[i].pad_cfg);
		if (pin_ptr[i].in_select)
			mxc_iomux_set_input(pin_ptr[i].in_select,
					pin_ptr[i].in_mode);
	}

	/* Camera low power */
	mxc_set_gpio_direction(MX51_PIN_CSI1_D8, 0);
	mxc_set_gpio_dataout(MX51_PIN_CSI1_D8, 0);
	mxc_set_gpio_direction(MX51_PIN_EIM_EB2, 0);    /* TO1 */
	mxc_set_gpio_dataout(MX51_PIN_EIM_EB2, 0);      /* TO1 */

	/* Camera reset */
	mxc_set_gpio_direction(MX51_PIN_CSI1_D9, 0);
	mxc_set_gpio_dataout(MX51_PIN_CSI1_D9, 1);
	mxc_set_gpio_direction(MX51_PIN_DI1_D1_CS, 0);

	mxc_set_gpio_direction(MX51_PIN_GPIO1_0, 1);	/* SD1 CD */
	mxc_set_gpio_direction(MX51_PIN_GPIO1_1, 1);	/* SD1 WP */

	/* EIM_D16 */
	/* osc_en is shared by SPDIF */
	mxc_set_gpio_direction(MX51_PIN_EIM_D16, 0);
	mxc_set_gpio_dataout(MX51_PIN_EIM_D16, 1);

	/* LCD related gpio */
	mxc_set_gpio_direction(MX51_PIN_DI1_D1_CS, 0);
	mxc_set_gpio_direction(MX51_PIN_DISPB2_SER_DIO, 0);
	mxc_set_gpio_dataout(MX51_PIN_DISPB2_SER_DIO, 0);

	/* GPS related gpio */
	mxc_set_gpio_direction(MX51_PIN_EIM_CS2, 0);
	mxc_set_gpio_dataout(MX51_PIN_EIM_CS2, 0);
	mxc_set_gpio_direction(MX51_PIN_EIM_CRE, 0);
	mxc_set_gpio_dataout(MX51_PIN_EIM_CRE, 0);
	mxc_set_gpio_dataout(MX51_PIN_EIM_CRE, 1);
}

/* workaround for ecspi chipselect pin may not keep correct level when idle */
void gpio_spi_chipselect_active(int cspi_mode, int status, int chipselect)
{
	switch (cspi_mode) {
	case 1:
		switch (chipselect) {
		case 0x1:
			mxc_request_iomux(MX51_PIN_CSPI1_SS0,
					  IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS0,
					  PAD_CTL_HYS_ENABLE |
					  PAD_CTL_PKE_ENABLE |
					  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);

			mxc_request_iomux(MX51_PIN_CSPI1_SS1,
					  IOMUX_CONFIG_GPIO);
			mxc_set_gpio_direction(MX51_PIN_CSPI1_SS1, 0);
			mxc_set_gpio_dataout(MX51_PIN_CSPI1_SS1, 2 & (~status));
			break;
		case 0x2:
			mxc_request_iomux(MX51_PIN_CSPI1_SS1,
					  IOMUX_CONFIG_ALT0);
			mxc_iomux_set_pad(MX51_PIN_CSPI1_SS1,
					  PAD_CTL_HYS_ENABLE |
					  PAD_CTL_PKE_ENABLE |
					  PAD_CTL_DRV_HIGH | PAD_CTL_SRE_FAST);

			mxc_request_iomux(MX51_PIN_CSPI1_SS0,
					  IOMUX_CONFIG_GPIO);
			mxc_set_gpio_direction(MX51_PIN_CSPI1_SS0, 0);
			mxc_set_gpio_dataout(MX51_PIN_CSPI1_SS0, 1 & (~status));
			break;
		default:
			break;
		}
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_chipselect_active);

void gpio_spi_chipselect_inactive(int cspi_mode, int status, int chipselect)
{
	switch (cspi_mode) {
	case 1:
		switch (chipselect) {
		case 0x1:
			mxc_free_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_ALT0);
			mxc_free_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_GPIO);
			break;
		case 0x2:
			mxc_free_iomux(MX51_PIN_CSPI1_SS0, IOMUX_CONFIG_GPIO);
			mxc_free_iomux(MX51_PIN_CSPI1_SS1, IOMUX_CONFIG_ALT0);
			break;
		default:
			break;
		}
		break;
	case 2:
		break;
	case 3:
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(gpio_spi_chipselect_inactive);

