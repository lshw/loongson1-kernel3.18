/*
 * Copyright (c) 2011 Zhang, Keguang <keguang.zhang@gmail.com>
 * Copyright (c) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * Modified from arch/mips/pnx833x/common/prom.c.
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/serial_reg.h>
#include <linux/delay.h>
#include <linux/ctype.h>

#include <video/ls1xfb.h>
#include <asm/bootinfo.h>

#include <loongson1.h>
#include <prom.h>

unsigned long ls1x_osc_clk;
EXPORT_SYMBOL(ls1x_osc_clk);

#if defined(CONFIG_LOONGSON1_LS1C) && defined(CONFIG_FB_LOONGSON1)
bool disable_gpio_58_77;
EXPORT_SYMBOL(disable_gpio_58_77);
#endif

int prom_argc;
char **prom_argv, **prom_envp;
unsigned long memsize, highmemsize;

char *prom_getenv(char *envname)
{
	char **env = prom_envp;
	int i;

	i = strlen(envname);

	while (*env) {
		if (strncmp(envname, *env, i) == 0 && *(*env + i) == '=')
			return *env + i + 1;
		env++;
	}

	return 0;
}

static inline unsigned long env_or_default(char *env, unsigned long dfl)
{
	char *str = prom_getenv(env);
	return str ? simple_strtol(str, 0, 0) : dfl;
}

void __init prom_init_cmdline(void)
{
	char *c = &(arcs_cmdline[0]);
	int i;

	for (i = 1; i < prom_argc; i++) {
		strcpy(c, prom_argv[i]);
		c += strlen(prom_argv[i]);
		if (i < prom_argc - 1)
			*c++ = ' ';
	}
	*c = 0;
}

void __init prom_init(void)
{
	void __iomem *uart_base;
	prom_argc = fw_arg0;
	prom_argv = (char **)fw_arg1;
	prom_envp = (char **)fw_arg2;

#if defined(CONFIG_LOONGSON1_LS1C)
	__raw_writel(__raw_readl(LS1X_MUX_CTRL0) & (~USBHOST_SHUT), LS1X_MUX_CTRL0);
	__raw_writel(__raw_readl(LS1X_MUX_CTRL1) & (~USBHOST_RSTN), LS1X_MUX_CTRL1);
	mdelay(60);
	/* reset stop */
	__raw_writel(__raw_readl(LS1X_MUX_CTRL1) | USBHOST_RSTN, LS1X_MUX_CTRL1);
#else
	/* 需要复位一次USB控制器，且复位时间要足够长，否则启动时莫名其妙的死机 */
	#if defined(CONFIG_LOONGSON1_LS1A)
	#define MUX_CTRL0 LS1X_MUX_CTRL0
	#define MUX_CTRL1 LS1X_MUX_CTRL1
	#elif defined(CONFIG_LOONGSON1_LS1B)
	#define MUX_CTRL0 LS1X_MUX_CTRL1
	#define MUX_CTRL1 LS1X_MUX_CTRL1
	#endif
//	__raw_writel(__raw_readl(MUX_CTRL0) | USB_SHUT, MUX_CTRL0);
//	__raw_writel(__raw_readl(MUX_CTRL1) & (~USB_RESET), MUX_CTRL1);
//	mdelay(10);
//	#if defined(CONFIG_USB_EHCI_HCD_LS1X) || defined(CONFIG_USB_OHCI_HCD_LS1X)
	/* USB controller enable and reset */
	__raw_writel(__raw_readl(MUX_CTRL0) & (~USB_SHUT), MUX_CTRL0);
	__raw_writel(__raw_readl(MUX_CTRL1) & (~USB_RESET), MUX_CTRL1);
	mdelay(60);
	/* reset stop */
	__raw_writel(__raw_readl(MUX_CTRL1) | USB_RESET, MUX_CTRL1);
//	#endif
#endif

	prom_init_cmdline();

#if defined(CONFIG_LOONGSON1_LS1C) && defined(CONFIG_FB_LOONGSON1)
	/*启用LCD的时候，gpio58到gpio73不能用
	 */
	if(strstr(arcs_cmdline, "video=ls1")){
		disable_gpio_58_77 = true;
	}else{
		disable_gpio_58_77 = false;
	}

#endif

	ls1x_osc_clk = env_or_default("osc_clk", OSC);
	memsize = env_or_default("memsize", DEFAULT_MEMSIZE);
	highmemsize = env_or_default("highmemsize", 0x0);

	if (strstr(arcs_cmdline, "console=ttyS5"))
	#if defined(CONFIG_LOONGSON1_LS1B) || defined(CONFIG_LOONGSON1_LS1C)
		uart_base = ioremap_nocache(LS1X_UART5_BASE, 0x0f);
	#else
		uart_base = ioremap_nocache(LS1X_UART0_BASE, 0x0f);
	#endif
	else if (strstr(arcs_cmdline, "console=ttyS3"))
		uart_base = ioremap_nocache(LS1X_UART3_BASE, 0x0f);
	else if (strstr(arcs_cmdline, "console=ttyS2"))
		uart_base = ioremap_nocache(LS1X_UART2_BASE, 0x0f);
	else if (strstr(arcs_cmdline, "console=ttyS1"))
		uart_base = ioremap_nocache(LS1X_UART1_BASE, 0x0f);
	else
		uart_base = ioremap_nocache(LS1X_UART0_BASE, 0x0f);
	setup_8250_early_printk_port((unsigned long)uart_base, 0, 0);

	pr_info("memsize=%ldMB, highmemsize=%ldMB\n", memsize, highmemsize);

#if defined(CONFIG_LOONGSON1_LS1B) && defined(CONFIG_FB_LOONGSON1)
	/* 只用于ls1b的lcd接口传vga接口时使用，如云终端，
	 因为使用vga时，pll通过计算难以得到合适的分频;给LS1X_CLK_PLL_FREQ和LS1X_CLK_PLL_DIV
	 寄存器一个固定的值 */
#include "video_modes.c"
	{
	int i;
	int default_xres = 800;
	int default_yres = 600;
	int default_refresh = 75;
	struct ls1b_vga *input_vga;
	extern struct ls1b_vga ls1b_vga_modes[];
	char *strp, *options, *end;

	options = strstr(arcs_cmdline, "video=ls1bvga:");
	if (options) {
		
		options += 14;
		/* ls1bvga:1920x1080-16@60 */
		for (i=0; i<strlen(options); i++)
			if (isdigit(*(options+i)))
				break;	/* 查找options字串中第一个数字时i的值 */
		if (i < 4) {
			default_xres = simple_strtoul(options+i, &end, 10);
			default_yres = simple_strtoul(end+1, NULL, 10);
			/* refresh */
			strp = strchr((const char *)options, '@');
			if (strp) {
				default_refresh = simple_strtoul(strp+1, NULL, 0);
			}
			if ((default_xres<=0 || default_xres>1920) || 
				(default_yres<=0 || default_yres>1080)) {
				pr_info("Warning: Resolution is out of range."
					"MAX resolution is 1920x1080@60Hz\n");
				default_xres = 800;
				default_yres = 600;
			}
		}
		for (input_vga=ls1b_vga_modes; input_vga->ls1b_pll_freq !=0; ++input_vga) {
//			if((input_vga->xres == default_xres) && (input_vga->yres == default_yres) && 
//				(input_vga->refresh == default_refresh)) {
			if ((input_vga->xres == default_xres) && (input_vga->yres == default_yres)) {
				break;
			}
		}
		if (input_vga->ls1b_pll_freq) {
			u32 pll, ctrl;
			u32 x, divisor;

			writel(input_vga->ls1b_pll_freq, LS1X_CLK_PLL_FREQ);
			writel(input_vga->ls1b_pll_div, LS1X_CLK_PLL_DIV);
			/* 计算ddr频率，更新串口分频 */
			pll = input_vga->ls1b_pll_freq;
			ctrl = input_vga->ls1b_pll_div & DIV_DDR;
			divisor = (12 + (pll & 0x3f)) * ls1x_osc_clk / 2
					+ ((pll >> 8) & 0x3ff) * ls1x_osc_clk / 1024 / 2;
			divisor = divisor / (ctrl >> DIV_DDR_SHIFT);
			divisor = divisor / 2 / (16*115200);
			x = readb(uart_base + UART_LCR);
			writeb(x | UART_LCR_DLAB, uart_base + UART_LCR);
			writeb(divisor & 0xff, uart_base + UART_DLL);
			writeb((divisor>>8) & 0xff, uart_base + UART_DLM);
			writeb(x & ~UART_LCR_DLAB, uart_base + UART_LCR);
		}
	}
	}
#endif
}

void __init prom_free_prom_memory(void)
{
}
