/*
 * Copyright (c) 2012 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/clkdev.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>

#include <loongson1.h>

extern unsigned long ls1x_osc_clk;
static DEFINE_SPINLOCK(_lock);

static int ls1x_pll_clk_enable(struct clk_hw *hw)
{
	return 0;
}

static void ls1x_pll_clk_disable(struct clk_hw *hw)
{
}

static unsigned long ls1x_pll_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	u32 pll, rate;

	pll = __raw_readl(LS1X_CLK_PLL_FREQ);
/*	rate = 12 + (pll & 0x3f) + (((pll >> 8) & 0x3ff) >> 10);
	rate *= ls1x_osc_clk;
	rate >>= 1;*/

#if defined(CONFIG_LOONGSON1_LS1C)
	rate = (((pll >> 8) & 0xff) + ((pll >> 16) & 0xff)) * ls1x_osc_clk / 4;
#else
	rate = (12 + (pll & 0x3f)) * ls1x_osc_clk / 2
			+ ((pll >> 8) & 0x3ff) * ls1x_osc_clk / 1024 / 2;
#endif

	return rate;
}

static const struct clk_ops ls1x_pll_clk_ops = {
	.enable = ls1x_pll_clk_enable,
	.disable = ls1x_pll_clk_disable,
	.recalc_rate = ls1x_pll_recalc_rate,
};

static struct clk *__init clk_register_pll(struct device *dev,
					   const char *name,
					   const char *parent_name,
					   unsigned long flags)
{
	struct clk_hw *hw;
	struct clk *clk;
	struct clk_init_data init;

	/* allocate the divider */
	hw = kzalloc(sizeof(struct clk_hw), GFP_KERNEL);
	if (!hw) {
		pr_err("%s: could not allocate clk_hw\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &ls1x_pll_clk_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name : NULL);
	init.num_parents = (parent_name ? 1 : 0);
	hw->init = &init;

	/* register the clock */
	clk = clk_register(dev, hw);

	if (IS_ERR(clk))
		kfree(hw);

	return clk;
}

static const char const *cpu_parents[] = { "cpu_clk_div", "osc_clk", };
static const char const *ahb_parents[] = { "ahb_clk_div", "osc_clk", };
static const char const *dc_parents[] = { "dc_clk_div", "osc_clk", };

void __init ls1x_clk_init(void)
{
	struct clk *clk;
	u32 ctrl;

	clk = clk_register_fixed_rate(NULL, "osc_clk", NULL, CLK_IS_ROOT,
				      ls1x_osc_clk);
	clk_register_clkdev(clk, "osc_clk", NULL);

	/* clock derived from 33 MHz OSC clk */
	clk = clk_register_pll(NULL, "pll_clk", "osc_clk", 0);
	clk_register_clkdev(clk, "pll_clk", NULL);

	/* clock derived from PLL clk */
	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ CPU CLK
	 *        \___ PLL ___ CPU DIV ___|     |
	 *                                |_____|
	 */
#if defined(CONFIG_LOONGSON1_LS1A)
	/* 由于目前loongson 1A CPU读取0xbfe78030 PLL寄存器有问题，
	   所以CPU的频率是通过PMON传进来的 */
	clk = clk_register_fixed_factor(NULL, "cpu_clk", "osc_clk", 0, 8, 1);
	clk_register_clkdev(clk, "cpu_clk", NULL);
#elif defined(CONFIG_LOONGSON1_LS1B)
	clk = clk_register_divider(NULL, "cpu_clk_div", "pll_clk",
				   CLK_GET_RATE_NOCACHE, LS1X_CLK_PLL_DIV,
				   DIV_CPU_SHIFT, DIV_CPU_WIDTH,
				   CLK_DIVIDER_ONE_BASED |
				   CLK_DIVIDER_ROUND_CLOSEST, &_lock);
	clk_register_clkdev(clk, "cpu_clk_div", NULL);
	clk = clk_register_mux(NULL, "cpu_clk", cpu_parents,
			       ARRAY_SIZE(cpu_parents),
			       CLK_SET_RATE_NO_REPARENT, LS1X_CLK_PLL_DIV,
			       BYPASS_CPU_SHIFT, BYPASS_CPU_WIDTH, 0, &_lock);
	clk_register_clkdev(clk, "cpu_clk", NULL);
#else
	ctrl = __raw_readl(LS1X_CLK_PLL_DIV);
	if (ctrl & DIV_CPU_SEL) {
		if(ctrl & DIV_CPU_EN) {
			clk = clk_register_divider(NULL, "cpu_clk", "pll_clk",
					CLK_SET_RATE_PARENT, LS1X_CLK_PLL_DIV, DIV_CPU_SHIFT,
					DIV_CPU_WIDTH, CLK_DIVIDER_ONE_BASED, &_lock);
		} else {
			clk = clk_register_fixed_factor(NULL, "cpu_clk", "pll_clk", 0, 1, 2);
		}
	}
	clk_register_clkdev(clk, "cpu_clk", NULL);
#endif

	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ DC  CLK
	 *        \___ PLL ___ DC  DIV ___|     |
	 *                                |_____|
	 */
#if defined(CONFIG_LOONGSON1_LS1A)
	/* 由于目前loongson 1A CPU读取0xbfe78030 PLL寄存器有问题，
	   所以BUS(DDR)的频率是通过PMON传进来的 */
#elif defined(CONFIG_LOONGSON1_LS1B)
	clk = clk_register_divider(NULL, "dc_clk_div", "pll_clk",
				   0, LS1X_CLK_PLL_DIV, DIV_DC_SHIFT,
				   DIV_DC_WIDTH, CLK_DIVIDER_ONE_BASED, &_lock);
	clk_register_clkdev(clk, "dc_clk_div", NULL);
	clk = clk_register_mux(NULL, "dc_clk", dc_parents,
			       ARRAY_SIZE(dc_parents),
			       CLK_SET_RATE_NO_REPARENT, LS1X_CLK_PLL_DIV,
			       BYPASS_DC_SHIFT, BYPASS_DC_WIDTH, 0, &_lock);
	clk_register_clkdev(clk, "dc_clk", NULL);
#else
	clk = clk_register_divider(NULL, "dc_clk", "pll_clk",
			CLK_SET_RATE_PARENT, LS1X_CLK_PLL_DIV, DIV_DC_SHIFT,
			DIV_DC_WIDTH, CLK_DIVIDER_ONE_BASED, &_lock);
	clk_register_clkdev(clk, "dc_clk", NULL);
#endif

	/*                                 _____
	 *         _______________________|     |
	 * OSC ___/                       | MUX |___ DDR CLK
	 *        \___ PLL ___ DDR DIV ___|     |
	 *                                |_____|
	 */
#if defined(CONFIG_LOONGSON1_LS1A)
	/* 由于目前loongson 1A CPU读取0xbfe78030 PLL寄存器有问题，
	   所以BUS(DDR)的频率是通过PMON传进来的 */
	clk = clk_register_fixed_factor(NULL, "ahb_clk", "osc_clk", 0, 4, 1);
#elif defined(CONFIG_LOONGSON1_LS1B)
	clk = clk_register_divider(NULL, "ahb_clk_div", "pll_clk",
				   0, LS1X_CLK_PLL_DIV, DIV_DDR_SHIFT,
				   DIV_DDR_WIDTH, CLK_DIVIDER_ONE_BASED,
				   &_lock);
	clk_register_clkdev(clk, "ahb_clk_div", NULL);
	clk = clk_register_mux(NULL, "ahb_clk", ahb_parents,
			       ARRAY_SIZE(ahb_parents),
			       CLK_SET_RATE_NO_REPARENT, LS1X_CLK_PLL_DIV,
			       BYPASS_DDR_SHIFT, BYPASS_DDR_WIDTH, 0, &_lock);
#else
	ctrl = __raw_readl(LS1X_CLK_PLL_FREQ) & 0x3;
	switch (ctrl) {
		case 0:
			clk = clk_register_fixed_factor(NULL, "ahb_clk", "cpu_clk", 0, 1, 2);
		break;
		case 1:
			clk = clk_register_fixed_factor(NULL, "ahb_clk", "cpu_clk", 0, 1, 4);
		break;
		case 2:
		case 3:
			clk = clk_register_fixed_factor(NULL, "ahb_clk", "cpu_clk", 0, 1, 3);
		break;
	}
#endif
	clk_register_clkdev(clk, "ahb_clk", NULL);
	clk_register_clkdev(clk, "stmmaceth", NULL);

	/* clock derived from AHB clk */
	/* APB clk is always half of the AHB clk */
	clk = clk_register_fixed_factor(NULL, "apb_clk", "ahb_clk", 0, 1,
					DIV_APB);
	clk_register_clkdev(clk, "apb_clk", NULL);
	clk_register_clkdev(clk, "ls1x_i2c", NULL);
	clk_register_clkdev(clk, "ls1x_pwmtimer", NULL);
	clk_register_clkdev(clk, "ls1x_spi", NULL);
	clk_register_clkdev(clk, "ls1x_wdt", NULL);
	clk_register_clkdev(clk, "serial8250", NULL);
	clk_register_clkdev(clk, "ls1x_sdio", NULL);

	clk = clk_register_fixed_rate(NULL, "ls1x_i2s_mclk", NULL, CLK_IS_ROOT,
				      22579200);
	clk_register_clkdev(clk, "ls1x_i2s_mclk", NULL);
}
