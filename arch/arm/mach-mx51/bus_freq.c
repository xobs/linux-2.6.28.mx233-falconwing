/*
 * Copyright 2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file bus_freq.c
 *
 * @brief A common API for the Freescale Semiconductor i.MXC CPUfreq module
 * and DVFS CORE module.
 *
 * The APIs are for setting bus frequency to low or high.
 *
 * @ingroup PM
 */

#include <linux/proc_fs.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <mach/clock.h>
#include <mach/hardware.h>
#include <linux/regulator/consumer.h>

#define LP_NORMAL_CLK			133000000
#define LP_MED_CLK			83125000
#define LP_APM_CLK   			24000000
#define NAND_LP_APM_CLK			12000000
#define DDR_LOW_FREQ_CLK		133000000
#define DDR_NORMAL_CLK			200000000
#define AXI_A_NORMAL_CLK		166250000
#define AXI_A_CLK_NORMAL_DIV		4
#define AXI_B_CLK_NORMAL_DIV		5
#define AHB_CLK_NORMAL_DIV		AXI_B_CLK_NORMAL_DIV
#define EMI_SLOW_CLK_NORMAL_DIV		AXI_B_CLK_NORMAL_DIV
#define NFC_CLK_NORMAL_DIV      	4

struct clk *ddr_clk;
struct clk *pll2;
struct clk *main_bus_clk;
struct clk *axi_a_clk;
struct clk *axi_b_clk;
struct clk *pll1_sw_clk;
struct clk *ddr_hf_clk;
struct clk *nfc_clk;
struct clk *ahb_clk;
struct clk *vpu_clk;
struct clk *vpu_core_clk;
struct clk *emi_slow_clk;
struct clk *ddr_clk;
struct clk *ipu_clk;
struct clk *periph_apm_clk;
struct clk *lp_apm;
struct clk *osc;
struct regulator *lp_regulator;
int low_bus_freq_mode;
int high_bus_freq_mode;

char *gp_reg_id = "SW1";
char *lp_reg_id = "SW2";

extern int lp_high_freq;
extern int lp_med_freq;
extern int dvfs_core_is_active;

int set_low_bus_freq(void)
{
	struct clk *p_clk;
	struct clk *amode_parent_clk;

	/*Change the DDR freq to 133Mhz. */
	clk_set_rate(ddr_hf_clk, clk_round_rate(ddr_hf_clk, DDR_LOW_FREQ_CLK));

	p_clk = clk_get_parent(periph_apm_clk);
	/* Make sure osc_clk is the parent of lp_apm. */
	clk_set_parent(lp_apm, osc);
	/* Set the parent of periph_apm_clk to be lp_apm */
	clk_set_parent(periph_apm_clk, lp_apm);

	amode_parent_clk = periph_apm_clk;

	p_clk = clk_get_parent(main_bus_clk);
	/* Set the parent of main_bus_clk to be periph_apm_clk */
	clk_set_parent(main_bus_clk, amode_parent_clk);

	clk_set_rate(axi_a_clk, LP_APM_CLK);
	clk_set_rate(axi_b_clk, LP_APM_CLK);
	clk_set_rate(ahb_clk, LP_APM_CLK);
	clk_set_rate(emi_slow_clk, LP_APM_CLK);
	clk_set_rate(nfc_clk, NAND_LP_APM_CLK);

	low_bus_freq_mode = 1;
	high_bus_freq_mode = 0;

	return 0;
}

int set_high_bus_freq(int high_bus_freq)
{
	if (clk_get_rate(main_bus_clk) == LP_APM_CLK) {

		clk_enable(pll2);

		/* Set the dividers before setting the parent clock. */
		clk_set_rate(axi_a_clk, LP_APM_CLK/AXI_A_CLK_NORMAL_DIV);
		clk_set_rate(axi_b_clk, LP_APM_CLK/AXI_B_CLK_NORMAL_DIV);
		clk_set_rate(ahb_clk, LP_APM_CLK/AHB_CLK_NORMAL_DIV);
		clk_set_rate(emi_slow_clk, LP_APM_CLK/EMI_SLOW_CLK_NORMAL_DIV);
		clk_set_rate(nfc_clk,
				clk_get_rate(emi_slow_clk)/NFC_CLK_NORMAL_DIV);
		/* Set the parent of main_bus_clk to be pll2 */
		clk_set_parent(main_bus_clk, pll2);

		/*Change the DDR freq to 200MHz*/
		clk_set_rate(ddr_hf_clk,
				clk_round_rate(ddr_hf_clk, DDR_NORMAL_CLK));

		low_bus_freq_mode = 0;
	}
	if (((clk_get_rate(ahb_clk) == LP_MED_CLK) && lp_high_freq) ||
						high_bus_freq) {
		/* Set to the high setpoint. */
		high_bus_freq_mode = 1;
		clk_set_rate(axi_a_clk,
				clk_round_rate(axi_a_clk, AXI_A_NORMAL_CLK));
		clk_set_rate(ahb_clk,
				clk_round_rate(axi_b_clk, LP_NORMAL_CLK));
		clk_set_rate(ddr_hf_clk,
				clk_round_rate(ddr_hf_clk, DDR_NORMAL_CLK));

	}
	if (lp_med_freq && !lp_high_freq && !high_bus_freq) {
		/* Set to the medium setpoint. */
		high_bus_freq_mode = 0;
		low_bus_freq_mode = 0;
		clk_set_rate(ddr_hf_clk,
				clk_round_rate(ddr_hf_clk, DDR_LOW_FREQ_CLK));
		clk_set_rate(axi_a_clk, clk_round_rate(axi_a_clk, LP_MED_CLK));
		clk_set_rate(ahb_clk, clk_round_rate(axi_b_clk, LP_MED_CLK));
	}
	return 0;
}

int low_freq_bus_used(void)
{
	if ((clk_get_usecount(ipu_clk) == 0)
	    && (clk_get_usecount(vpu_clk) == 0)
	    && (lp_high_freq == 0)
	    && (lp_med_freq == 0))
		return 1;
	else
		return 0;
}

/*!
 * This is the probe routine for the bus frequency driver.
 *
 * @param   pdev   The platform device structure
 *
 * @return         The function returns 0 on success
 *
 */
static int __devinit busfreq_probe(struct platform_device *pdev)
{
	main_bus_clk = clk_get(NULL, "main_bus_clk");
	if (IS_ERR(main_bus_clk)) {
		printk(KERN_DEBUG "%s: failed to get main_bus_clk\n",
		       __func__);
		return PTR_ERR(main_bus_clk);
	}

	pll2 = clk_get(NULL, "pll2");
	if (IS_ERR(pll2)) {
		printk(KERN_DEBUG "%s: failed to get pll2\n", __func__);
		return PTR_ERR(pll2);
	}

	axi_a_clk = clk_get(NULL, "axi_a_clk");
	if (IS_ERR(axi_a_clk)) {
		printk(KERN_DEBUG "%s: failed to get axi_a_clk\n",
		       __func__);
		return PTR_ERR(axi_a_clk);
	}

	axi_b_clk = clk_get(NULL, "axi_b_clk");
	if (IS_ERR(axi_b_clk)) {
		printk(KERN_DEBUG "%s: failed to get axi_b_clk\n",
		       __func__);
		return PTR_ERR(axi_b_clk);
	}

	ddr_hf_clk = clk_get(NULL, "ddr_hf_clk");
	if (IS_ERR(ddr_hf_clk)) {
		printk(KERN_DEBUG "%s: failed to get ddr_hf_clk\n",
		       __func__);
		return PTR_ERR(ddr_hf_clk);
	}

	emi_slow_clk = clk_get(NULL, "emi_slow_clk");
	if (IS_ERR(emi_slow_clk)) {
		printk(KERN_DEBUG "%s: failed to get emi_slow_clk\n",
		       __func__);
		return PTR_ERR(emi_slow_clk);
	}

	nfc_clk = clk_get(NULL, "nfc_clk");
	if (IS_ERR(nfc_clk)) {
		printk(KERN_DEBUG "%s: failed to get nfc_clk\n",
		       __func__);
		return PTR_ERR(nfc_clk);
	}

	ahb_clk = clk_get(NULL, "ahb_clk");
	if (IS_ERR(ahb_clk)) {
		printk(KERN_DEBUG "%s: failed to get ahb_clk\n",
		       __func__);
		return PTR_ERR(ahb_clk);
	}

	vpu_core_clk = clk_get(NULL, "vpu_core_clk");
	if (IS_ERR(vpu_core_clk)) {
		printk(KERN_DEBUG "%s: failed to get vpu_core_clk\n",
		       __func__);
		return PTR_ERR(vpu_core_clk);
	}

	ddr_clk = clk_get(NULL, "ddr_clk");
	if (IS_ERR(ddr_clk)) {
		printk(KERN_DEBUG "%s: failed to get ddr_clk\n",
		       __func__);
		return PTR_ERR(ddr_clk);
	}

	pll1_sw_clk = clk_get(NULL, "pll1_sw_clk");
	if (IS_ERR(pll1_sw_clk)) {
		printk(KERN_DEBUG "%s: failed to get pll1_sw_clk\n",
		       __func__);
		return PTR_ERR(pll1_sw_clk);
	}

	ipu_clk = clk_get(NULL, "ipu_clk");
	if (IS_ERR(ipu_clk)) {
		printk(KERN_DEBUG "%s: failed to get ipu_clk\n",
		       __func__);
		return PTR_ERR(ipu_clk);
	}

	vpu_clk = clk_get(NULL, "vpu_clk");
	if (IS_ERR(vpu_clk)) {
		printk(KERN_DEBUG "%s: failed to get vpu_clk\n",
		       __func__);
		return PTR_ERR(vpu_clk);
	}

	periph_apm_clk = clk_get(NULL, "periph_apm_clk");
	if (IS_ERR(periph_apm_clk)) {
		printk(KERN_DEBUG "%s: failed to get periph_apm_clk\n",
		       __func__);
		return PTR_ERR(periph_apm_clk);
	}

	lp_apm = clk_get(NULL, "lp_apm");
	if (IS_ERR(lp_apm)) {
		printk(KERN_DEBUG "%s: failed to get lp_apm\n",
		       __func__);
		return PTR_ERR(lp_apm);
	}

	osc = clk_get(NULL, "osc");
	if (IS_ERR(osc)) {
		printk(KERN_DEBUG "%s: failed to get osc\n", __func__);
		return PTR_ERR(osc);
	}

	low_bus_freq_mode = 0;
	high_bus_freq_mode = 0;

	return 0;
}

static struct platform_driver busfreq_driver = {
	.driver = {
		   .name = "busfreq",
		   },
	.probe = busfreq_probe,
};

/*!
 * Initialise the busfreq_driver.
 *
 * @return  The function always returns 0.
 */

static int __init busfreq_init(void)
{
	if (platform_driver_register(&busfreq_driver) != 0) {
		printk(KERN_ERR "busfreq_driver register failed\n");
		return -ENODEV;
	}

	printk(KERN_INFO "Bus freq driver module loaded\n");
	return 0;
}

static void __exit busfreq_cleanup(void)
{
	/* Unregister the device structure */
	platform_driver_unregister(&busfreq_driver);
}

module_init(busfreq_init);
module_exit(busfreq_cleanup);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("BusFreq driver");
MODULE_LICENSE("GPL");

