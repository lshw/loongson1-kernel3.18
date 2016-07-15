/*
 *  Copyright (c) 2012 Tang, Haifeng <tanghaifeng-gz@loongson.cn> <pengren.mcu@qq.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#ifndef __ASM_MACH_LS1XFB_H
#define __ASM_MACH_LS1XFB_H

#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include <asm/mach-loongson1/loongson1.h>

/*
 * Buffer pixel format
 */
#define PIX_FMT_RGB444		1
#define PIX_FMT_RGB1555		2
#define PIX_FMT_RGB565		3
#define PIX_FMT_RGB888PACK	4
#define PIX_FMT_RGB888UNPACK	5
#define PIX_FMT_RGBA888		6
#define PIX_FMT_PSEUDOCOLOR	20

#define DEFAULT_FB_SIZE	(800 * 600 * 4)

/*
 * LS1X LCD controller private state.
 */
struct ls1xfb_info;

struct ls1xfb_info {
	struct device		*dev;
	struct clk		*clk;
	struct fb_info		*info;

	struct i2c_adapter *i2c_adapter;
	unsigned char *edid;

	void __iomem		*reg_base;
	dma_addr_t		fb_start_dma;
	u32			pseudo_palette[16];

	int			pix_fmt;
	unsigned		de_mode:1;
};

/*
 * LS1X fb machine information
 */
struct ls1xfb_mach_info {
	char	id[16];

	int	i2c_bus_num;
	int		num_modes;
	struct fb_videomode *modes;

	/*
	 * Pix_fmt
	 */
	unsigned	pix_fmt;

	/*
	 * Dumb panel -- configurable output signal polarity.
	 */
	unsigned	invert_pixclock:1;
	unsigned	invert_pixde:1;
	unsigned	de_mode:1;
	unsigned	enable_lcd:1;
};

#ifdef CONFIG_LOONGSON1_LS1B
struct ls1b_vga {
	unsigned int xres;
	unsigned int yres;
	unsigned int refresh;
	unsigned int ls1b_pll_freq;
	unsigned int ls1b_pll_div;
};
#endif

#if defined(CONFIG_FB_LS1X_I2C)
extern int ls1xfb_probe_i2c_connector(struct fb_info *info, u8 **out_edid);
#endif

#endif /* __ASM_MACH_LS1XFB_H */
