/*
 *  Copyright (c) 2012 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#ifndef __LS1XFB_H__
#define __LS1XFB_H__

#define LS1X_DC0_BASE			0x1c301240
#define LS1X_DC1_BASE			0x1c301250

#define LS1X_DC_REGS(x) \
		((void __iomem *)KSEG1ADDR(LS1X_DC_BASE + (x)))

/* Frame Buffer configuration */
#define LS1X_FB_CONF			0x0
#define LS1X_FB_CONF_RESET			(0x1 << 20)
#define LS1X_FB_CONF_GAMMA_EN			(0x1 << 12)
#define LS1X_FB_CONF_SP			(0x1 << 9)
#define LS1X_FB_CONF_OUT_EN			(0x1 << 8)
#define LS1X_FB_CONF_FORMAT			(0x7 << 0)

/* Frame buffer address0/address1 */
#define LS1X_FB_ADDR0			0x20
#define LS1X_FB_ADDR1			0x340

/* Frame buffer stride */
#define LS1X_FB_STRIDE			0x40
/* Frame buffer origin */
#define LS1X_FB_ORIGIN			0x60

/* Display Dither Configuration */
#define LS1X_FB_DITHER_CONF			0x120
#define LS1X_FB_DITHER_CONF_EN			(0x1 << 31)
#define LS1X_FB_DITHER_CONF_REDSIZE			(0xf << 16)
#define LS1X_FB_DITHER_CONF_GREENSIZE			(0xf << 8)
#define LS1X_FB_DITHER_CONF_BLUESIZE			(0xf << 0)

/* Display Dither Table */
#define LS1X_FB_DITHER_TAB_LOW			0x140
#define LS1X_FB_DITHER_TAB_LOWY_1_X3			(0xf << 28)
#define LS1X_FB_DITHER_TAB_LOWY_1_X2			(0xf << 24)
#define LS1X_FB_DITHER_TAB_LOWY_1_X1			(0xf << 20)
#define LS1X_FB_DITHER_TAB_LOWY_1_X0			(0xf << 16)
#define LS1X_FB_DITHER_TAB_LOWY_0_X3			(0xf << 12)
#define LS1X_FB_DITHER_TAB_LOWY_0_X2			(0xf << 8)
#define LS1X_FB_DITHER_TAB_LOWY_0_X1			(0xf << 4)
#define LS1X_FB_DITHER_TAB_LOWY_0_X0			(0xf << 0)

#define LS1X_FB_DITHER_TAB_HIGH			0x160
#define LS1X_FB_DITHER_TAB_LOWY_3_X3			(0xf << 28)
#define LS1X_FB_DITHER_TAB_LOWY_3_X2			(0xf << 24)
#define LS1X_FB_DITHER_TAB_LOWY_3_X1			(0xf << 20)
#define LS1X_FB_DITHER_TAB_LOWY_3_X0			(0xf << 16)
#define LS1X_FB_DITHER_TAB_LOWY_2_X3			(0xf << 12)
#define LS1X_FB_DITHER_TAB_LOWY_2_X2			(0xf << 8)
#define LS1X_FB_DITHER_TAB_LOWY_2_X1			(0xf << 4)
#define LS1X_FB_DITHER_TAB_LOWY_2_X0			(0xf << 0)

/* Panel configuration */
#define LS1X_FB_PANEL_CONF			0x180
#define LS1X_FB_PANEL_CONF_CLK_POL			(0x1 << 9)
#define LS1X_FB_PANEL_CONF_CLK			(0x1 << 8)	/* 无效？ */
//#define LS1X_FB_PANEL_CONF_DE_POL			(0x1 << 5)	/* ?? */
#define LS1X_FB_PANEL_CONF_DE_POL			(0x1 << 1)
#define LS1X_FB_PANEL_CONF_DE			(0x1 << 0)	/* 无效？ */

/*  */
#define LS1X_FB_PANEL_TIMING			0x1a0

/* HDisplay */
#define LS1X_FB_HDISPLAY			0x1c0
#define LS1X_FB_HDISPLAY_TOTAL			(0xfff << 16)
#define LS1X_FB_HDISPLAY_END			(0xfff << 0)

/* HSync */
#define LS1X_FB_HSYNC			0x1e0
#define LS1X_FB_HSYNC_POL			(0x1 << 31)
#define LS1X_FB_HSYNC_PULSE			(0x1 << 30)
#define LS1X_FB_HSYNC_END			(0xfff << 16)
#define LS1X_FB_HSYNC_START			(0xfff << 0)

/* VDisplay */
#define LS1X_FB_VDISPLAY			0x240
#define LS1X_FB_VDISPLAY_TOTAL			(0x7ff << 16)
#define LS1X_FB_VDISPLAY_END			(0x7ff << 0)

/* VSync */
#define LS1X_FB_VSYNC			0x260
#define LS1X_FB_VSYNC_POL			(0x1 << 31)
#define LS1X_FB_VSYNC_PULSE			(0x1 << 30)
#define LS1X_FB_VSYNC_END			(0xfff << 16)
#define LS1X_FB_VSYNC_START			(0xfff << 0)

#endif /* __LS1XFB_H__ */
