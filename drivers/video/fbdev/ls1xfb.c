/*
 *  Copyright (c) 2012 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 *  Based partly on pxa168fb.c
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/serial_reg.h>
#include <video/ls1xfb.h>

#include <asm/mach-loongson1/loongson1.h>
#include <asm/mach-loongson1/regs-clk.h>
#include "ls1xfb.h"

#define DRIVER_NAME_0 "ls1afb"
#define DRIVER_NAME_1 "ls1bfb"
#define DRIVER_NAME_2 "ls1xfb"
#define DRIVER_NAME_3 "ls1avga"
#define DRIVER_NAME_4 "ls1bvga"
#define DRIVER_NAME_5 "ls1xvga"

/* 如果PMON已配置好LCD控制器，这里可以不用再次初始化，可以避免屏幕切换时的花屏 */
#define INIT_AGAIN
#ifdef INIT_AGAIN
#define writel_reg(val, addr)	writel(val, addr);
#else
#define writel_reg(val, addr)
#endif

#define DEFAULT_REFRESH		75	/* Hz */
#define DEFAULT_XRES	800
#define DEFAULT_YRES	600
#define DEFAULT_BPP	16
static u32 default_refresh = DEFAULT_REFRESH;
static u32 default_xres = DEFAULT_XRES;
static u32 default_yres = DEFAULT_YRES;
static u32 default_bpp = 0;
static int vga_mode = 0;

static int determine_best_pix_fmt(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 12:
			return PIX_FMT_RGB444;
			break;
		case 15:
			return PIX_FMT_RGB1555;
			break;
		case 16:
			return PIX_FMT_RGB565;
			break;
		case 24:
		case 32:
			return PIX_FMT_RGBA888;
			break;
		default:
			return PIX_FMT_RGB565;
			break;
	}
	return -EINVAL;
}

static void set_pix_fmt(struct fb_var_screeninfo *var, int pix_fmt)
{
	switch (pix_fmt) {
	case PIX_FMT_RGB444:
		var->bits_per_pixel = 16;
		var->red.offset = 8;    var->red.length = 4;
		var->green.offset = 4;   var->green.length = 4;
		var->blue.offset = 0;    var->blue.length = 4;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB1555:
		var->bits_per_pixel = 16;
		var->red.offset = 10;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 5;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 0; var->transp.length = 0;
		break;
	case PIX_FMT_RGB565:
		var->bits_per_pixel = 16;
		var->red.offset = 11;    var->red.length = 5;
		var->green.offset = 5;   var->green.length = 6;
		var->blue.offset = 0;    var->blue.length = 5;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB888PACK:
		var->bits_per_pixel = 24;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 0;  var->transp.length = 0;
		break;
	case PIX_FMT_RGB888UNPACK:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	case PIX_FMT_RGBA888:
		var->bits_per_pixel = 32;
		var->red.offset = 16;    var->red.length = 8;
		var->green.offset = 8;   var->green.length = 8;
		var->blue.offset = 0;    var->blue.length = 8;
		var->transp.offset = 24; var->transp.length = 8;
		break;
	}
}

static int ls1xfb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	int pix_fmt, mode_valid = 0;

	/*
	 * Determine which pixel format we're going to use.
	 */
	pix_fmt = determine_best_pix_fmt(var);
	if (pix_fmt < 0)
		return pix_fmt;
	set_pix_fmt(var, pix_fmt);
	fbi->pix_fmt = pix_fmt;

#if 0
	if (!info->monspecs.hfmax || !info->monspecs.vfmax ||
	    !info->monspecs.dclkmax || !fb_validate_mode(var, info))
		mode_valid = 1;

	/* calculate modeline if supported by monitor */
	if (!mode_valid && info->monspecs.gtf) {
		if (!fb_get_mode(FB_MAXTIMINGS, 0, var, info))
			mode_valid = 1;
	}
#endif
	if (!mode_valid) {
		const struct fb_videomode *mode;

		mode = fb_find_best_mode(var, &info->modelist);
		if (mode) {
//			ls1x_update_var(var, mode);
			fb_videomode_to_var(var, mode);
			mode_valid = 1;
		}
	}

	if (!mode_valid && info->monspecs.modedb_len)
		return -EINVAL;

	/*
	 * Basic geometry sanity checks.
	 */
	if (var->xoffset + var->xres > var->xres_virtual) {
		return -EINVAL;
//		var->xres_virtual = var->xoffset + var->xres;
	}
	if (var->yoffset + var->yres > var->yres_virtual) {
		return -EINVAL;
//		var->yres_virtual = var->yoffset + var->yres;
	}
	if (var->xres + var->right_margin +
	    var->hsync_len + var->left_margin > 3000)
		return -EINVAL;
	if (var->yres + var->lower_margin +
	    var->vsync_len + var->upper_margin > 2000)
		return -EINVAL;

	/*
	 * Check size of framebuffer.
	 */
	if (var->xres_virtual * var->yres_virtual *
	    (var->bits_per_pixel >> 3) > info->fix.smem_len)
		return -EINVAL;
	return 0;
}

#ifdef CONFIG_LOONGSON1_LS1B
/* only for 1B vga */
static void set_uart_clock_divider(void)
{
	u32 pll, ctrl, rate, divisor;
	u32 x;
	int i;
	int ls1b_uart_base[] = {
		LS1X_UART0_BASE, LS1X_UART1_BASE, LS1X_UART2_BASE,
		LS1X_UART3_BASE, LS1X_UART4_BASE, LS1X_UART5_BASE,
		LS1X_UART6_BASE, LS1X_UART7_BASE, LS1X_UART8_BASE,
		LS1X_UART9_BASE, LS1X_UART10_BASE, LS1X_UART11_BASE
	};
	#define UART_PORT(id, offset)	(u8 *)(KSEG1ADDR(ls1b_uart_base[id] + offset))

	pll = readl(LS1X_CLK_PLL_FREQ);
	ctrl = readl(LS1X_CLK_PLL_DIV) & DIV_DDR;
	rate = (12 + (pll & 0x3f)) * OSC / 2
			+ ((pll >> 8) & 0x3ff) * OSC / 1024 / 2;
	rate = rate / (ctrl >> DIV_DDR_SHIFT);
	divisor = rate / 2 / (16*115200);
	
	for (i=0; i<12; i++) {
		x = readb(UART_PORT(i, UART_LCR));
		writeb(x | UART_LCR_DLAB, UART_PORT(i, UART_LCR));
	
		writeb(divisor & 0xff, UART_PORT(i, UART_DLL));
		writeb((divisor>>8) & 0xff, UART_PORT(i, UART_DLM));
	
		writeb(x & ~UART_LCR_DLAB, UART_PORT(i, UART_LCR));
	}
}

static void set_clock_divider_forls1bvga(struct ls1xfb_info *fbi,
			      const struct fb_videomode *m)
{
	struct ls1b_vga *input_vga;
	extern struct ls1b_vga ls1b_vga_modes[];

	for (input_vga=ls1b_vga_modes; input_vga->ls1b_pll_freq !=0; ++input_vga) {
//		if((input_vga->xres == m->xres) && (input_vga->yres == m->yres) && 
//			(input_vga->refresh == m->refresh)) {
		if ((input_vga->xres == m->xres) && (input_vga->yres == m->yres)) {
			break;
		}
	}
	if (input_vga->ls1b_pll_freq) {
		writel(input_vga->ls1b_pll_freq, LS1X_CLK_PLL_FREQ);
		writel(input_vga->ls1b_pll_div, LS1X_CLK_PLL_DIV);
		set_uart_clock_divider();
	}
}
#endif	//#ifdef CONFIG_LOONGSON1_LS1B

#ifdef CONFIG_LOONGSON1_LS1A
int caclulatefreq(unsigned int sys_clk, unsigned int pclk)
{
/* N值和OD值选择不正确会造成系统死机，莫名其妙。OD=2^PIX12 */
	unsigned int N = 4, PIX12 = 2, OD = 4;
	unsigned int M = 0, FRAC = 0;
	unsigned long tmp1, tmp2;

	while (1) {
		tmp2 = pclk * N * OD;
		M = tmp2 / sys_clk;
		if (M <= 1) {
			N++;
		} else {
			tmp1 = sys_clk * M;
			if (tmp2 < tmp1) {
				unsigned int tmp3;
				tmp3 = tmp1; tmp1 = tmp2; tmp2 = tmp3;
			}
			if ((tmp2 - tmp1) > 16384) {
				if (N < 15 ) {
					N++;
				} else {
					N = 15; PIX12++; OD *= 2;
					if (PIX12 > 3) {
						printk(KERN_WARNING "Warning: \
								clock source is out of range.\n");
						break;
					}
				}
			}
			else {
				FRAC = ((tmp2 - tmp1) * 262144) / sys_clk;
				break;
			}
		}
	}
//	printk("tmp2-tmp1=%d FRAC=%d\n", tmp2 - tmp1, FRAC);
//	printk("PIX12=%d N=%d M=%d\n", PIX12, N, M);
	return ((FRAC<<14) + (PIX12<<12) + (N<<8) + M);
}
#endif	//#ifdef CONFIG_LOONGSON1_LS1A

static void set_clock_divider(struct ls1xfb_info *fbi,
			      const struct fb_videomode *m)
{
	int divider_int;
	int needed_pixclk;
	u64 div_result;

	/*
	 * Notice: The field pixclock is used by linux fb
	 * is in pixel second. E.g. struct fb_videomode &
	 * struct fb_var_screeninfo
	 */

	/*
	 * Check input values.
	 */
	if (!m || !m->pixclock || !m->refresh) {
		dev_err(fbi->dev, "Input refresh or pixclock is wrong.\n");
		return;
	}

	/*
	 * Calc divider according to refresh rate.
	 */
	div_result = 1000000000000ll;
	do_div(div_result, m->pixclock);
	needed_pixclk = (u32)div_result;

#if defined(CONFIG_LOONGSON1_LS1A)
	#define PLL_CTRL(x)		(ioremap((x), 4))
	/* 设置gpu时钟频率为200MHz */
	divider_int = caclulatefreq(OSC/1000, 200000);
	writel(divider_int, PLL_CTRL(LS1X_GPU_PLL_CTRL));
	/* 像素时钟 */
	divider_int = caclulatefreq(OSC/1000, needed_pixclk/1000);
	writel(divider_int, PLL_CTRL(LS1X_PIX1_PLL_CTRL));
	writel(divider_int, PLL_CTRL(LS1X_PIX2_PLL_CTRL));
#elif defined(CONFIG_LOONGSON1_LS1B)
	divider_int = clk_get_rate(fbi->clk) / needed_pixclk / 4;
	/* check whether divisor is too small. */
	if (divider_int < 1) {
		dev_warn(fbi->dev, "Warning: clock source is too slow."
				"Try smaller resolution\n");
		divider_int = 1;
	}
	else if(divider_int > 15) {
		dev_warn(fbi->dev, "Warning: clock source is too fast."
				"Try smaller resolution\n");
		divider_int = 15;
	}

	/*
	 * Set setting to reg.
	 */
	{
		u32 regval = 0;
		regval = readl(LS1X_CLK_PLL_DIV);
		regval |= 0x00003000;	//dc_bypass 置1
		regval &= ~0x00000030;	//dc_rst 置0
		regval &= ~(0x1f<<26);	//dc_div 清零
		regval |= divider_int << 26;
		writel(regval, LS1X_CLK_PLL_DIV);
		regval &= ~0x00001000;	//dc_bypass 置0
		writel(regval, LS1X_CLK_PLL_DIV);
	}
#elif defined(CONFIG_LOONGSON1_LS1C)
	divider_int = clk_get_rate(fbi->clk) / needed_pixclk;
	/* check whether divisor is too small. */
	if (divider_int < 2) {
		dev_warn(fbi->dev, "Warning: clock source is too slow."
				"Try smaller resolution\n");
		divider_int = 1;
	}
	else if(divider_int > 127) {
		dev_warn(fbi->dev, "Warning: clock source is too fast."
				"Try smaller resolution\n");
		divider_int = 0x7f;
	}

	/*
	 * Set setting to reg.
	 */
	{
		u32 regval = 0;
		regval = readl(LS1X_CLK_PLL_DIV);
		/* 注意：首先需要把分频使能位清零 */
		writel(regval & ~DIV_DC_EN, LS1X_CLK_PLL_DIV);
		regval |= DIV_DC_EN | DIV_DC_SEL_EN | DIV_DC_SEL;
		regval &= ~DIV_DC;
		regval |= divider_int << DIV_DC_SHIFT;
		writel(regval, LS1X_CLK_PLL_DIV);
	}
#endif
}

static void set_graphics_start(struct fb_info *info, int xoffset, int yoffset)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;
	int pixel_offset;
	unsigned long addr;

	pixel_offset = (yoffset * var->xres_virtual) + xoffset;

	addr = fbi->fb_start_dma + (pixel_offset * (var->bits_per_pixel >> 3));
	writel(addr, fbi->reg_base + LS1X_FB_ADDR0);
	writel(addr, fbi->reg_base + LS1X_FB_ADDR1);
}

static void set_dumb_panel_control(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	struct ls1xfb_mach_info *mi = fbi->dev->platform_data;
	u32 x;

	/*
	 * Preserve enable flag.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF) & 0x80001110;

	/* PAN_CONFIG寄存器最高位需要置1，否则lcd时钟延时一段时间才会有输出 */
	if (unlikely(vga_mode)) {
		/* have to set 0x80001310 */
		writel_reg(x | 0x80001310, fbi->reg_base + LS1X_FB_PANEL_CONF);
	} else {
		x |= mi->invert_pixde ? LS1X_FB_PANEL_CONF_DE_POL : 0;
		x |= mi->invert_pixclock ? LS1X_FB_PANEL_CONF_CLK_POL : 0;
		x |= mi->de_mode ? LS1X_FB_PANEL_CONF_DE : 0;
		writel_reg(x | 0x80001010, fbi->reg_base + LS1X_FB_PANEL_CONF);
	}

	if (!mi->de_mode) {
		x = readl(fbi->reg_base + LS1X_FB_HSYNC) & ~LS1X_FB_HSYNC_POL;
		x |= (info->var.sync & FB_SYNC_HOR_HIGH_ACT) ? LS1X_FB_HSYNC_POL : 0;
		writel_reg(x, fbi->reg_base + LS1X_FB_HSYNC);

		x = readl(fbi->reg_base + LS1X_FB_VSYNC) & ~LS1X_FB_VSYNC_POL;
		x |= (info->var.sync & FB_SYNC_VERT_HIGH_ACT) ? LS1X_FB_VSYNC_POL : 0;
		writel_reg(x, fbi->reg_base + LS1X_FB_VSYNC);
	}
}

static void set_dumb_screen_dimensions(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *v = &info->var;
	int x;
	int y;

	x = v->xres + v->right_margin + v->hsync_len + v->left_margin;
	y = v->yres + v->lower_margin + v->vsync_len + v->upper_margin;

	writel_reg((readl(fbi->reg_base + LS1X_FB_HDISPLAY) & ~LS1X_FB_HDISPLAY_TOTAL) | (x << 16), 
		fbi->reg_base + LS1X_FB_HDISPLAY);
	writel_reg((readl(fbi->reg_base + LS1X_FB_VDISPLAY) & ~LS1X_FB_HDISPLAY_TOTAL) | (y << 16), 
		fbi->reg_base + LS1X_FB_VDISPLAY);
}

static int ls1xfb_set_par(struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;
	struct fb_videomode mode;
	u32 x;
	struct ls1xfb_mach_info *mi;

	mi = fbi->dev->platform_data;

	/*
	 * Set additional mode info.
	 */
	info->fix.visual = FB_VISUAL_TRUECOLOR;
	info->fix.line_length = var->xres_virtual * var->bits_per_pixel / 8;
//	info->fix.ypanstep = var->yres;

	/*
	 * Disable panel output while we setup the display.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF);
	writel_reg(x & ~LS1X_FB_PANEL_CONF_DE, fbi->reg_base + LS1X_FB_PANEL_CONF);

	/*
	 * convet var to video mode
	 */
	fb_var_to_videomode(&mode, &info->var);

	/* Calculate clock divisor. */
#ifdef CONFIG_LOONGSON1_LS1B
	if (unlikely(vga_mode)) {
		set_clock_divider_forls1bvga(fbi, &mode);
	}
	else 
#endif
	{
		set_clock_divider(fbi, &mode);
	}

	/*
	 * Configure dumb panel ctrl regs & timings.
	 */
	set_dumb_panel_control(info);
	set_dumb_screen_dimensions(info);

	writel_reg((readl(fbi->reg_base + LS1X_FB_HDISPLAY) & ~LS1X_FB_HDISPLAY_END) | (var->xres),
		fbi->reg_base + LS1X_FB_HDISPLAY);	/* 显示屏一行中显示区的像素数 */
	writel_reg((readl(fbi->reg_base + LS1X_FB_VDISPLAY) & ~LS1X_FB_VDISPLAY_END) | (var->yres),
		fbi->reg_base + LS1X_FB_VDISPLAY);	/* 显示屏中显示区的行数 */

	if (mi->de_mode) {
		writel_reg(0x00000000, fbi->reg_base + LS1X_FB_HSYNC);
		writel_reg(0x00000000, fbi->reg_base + LS1X_FB_VSYNC);
	} else {
		writel_reg((readl(fbi->reg_base + LS1X_FB_HSYNC) & 0xc0000000) | 0x40000000 | 
				((var->right_margin + var->xres + var->hsync_len) << 16) | 
				(var->right_margin + var->xres),
				fbi->reg_base + LS1X_FB_HSYNC);
		writel_reg((readl(fbi->reg_base + LS1X_FB_VSYNC) & 0xc0000000) | 0x40000000 | 
				((var->lower_margin + var->yres + var->vsync_len) << 16) | 
				(var->lower_margin + var->yres),
				fbi->reg_base + LS1X_FB_VSYNC);
	}

	/*
	 * Configure global panel parameters.
	 */
	x = readl(fbi->reg_base + LS1X_FB_CONF) & ~LS1X_FB_CONF_FORMAT;
	if (fbi->pix_fmt > 3) {
		writel_reg(x | LS1X_FB_CONF_RESET | 4, fbi->reg_base + LS1X_FB_CONF);
	}
	else {
		writel_reg(x | LS1X_FB_CONF_RESET | fbi->pix_fmt, fbi->reg_base + LS1X_FB_CONF);
	}

#if defined(CONFIG_LOONGSON1_LS1C)
	writel_reg((info->fix.line_length + 0x7f) & ~0x7f, fbi->reg_base + LS1X_FB_STRIDE);
#else
	writel_reg((info->fix.line_length + 0xff) & ~0xff, fbi->reg_base + LS1X_FB_STRIDE);
#endif
	writel_reg(0, fbi->reg_base + LS1X_FB_ORIGIN);

	/*
	 * Re-enable panel output.
	 */
	x = readl(fbi->reg_base + LS1X_FB_PANEL_CONF);
	writel_reg(x | LS1X_FB_PANEL_CONF_DE, fbi->reg_base + LS1X_FB_PANEL_CONF);

	return 0;
}

static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	return ((chan & 0xffff) >> (16 - bf->length)) << bf->offset;
}

static int
ls1xfb_setcolreg(unsigned int regno, unsigned int red, unsigned int green,
		 unsigned int blue, unsigned int trans, struct fb_info *info)
{
	struct ls1xfb_info *fbi = info->par;
	u32 val;

	if (info->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
					7471 * blue) >> 16;

	if (info->fix.visual == FB_VISUAL_TRUECOLOR && regno < 16) {
		val =  chan_to_field(red,   &info->var.red);
		val |= chan_to_field(green, &info->var.green);
		val |= chan_to_field(blue , &info->var.blue);
		fbi->pseudo_palette[regno] = val;
	}

	return 0;
}

static int ls1xfb_pan_display(struct fb_var_screeninfo *var,
				struct fb_info *info)
{
	set_graphics_start(info, var->xoffset, var->yoffset);

	return 0;
}

static struct fb_ops ls1xfb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= ls1xfb_check_var,
	.fb_set_par	= ls1xfb_set_par,
	.fb_setcolreg	= ls1xfb_setcolreg,
//	.fb_blank	= ls1xfb_blank,
	.fb_pan_display	= ls1xfb_pan_display,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
//	.fb_ioctl	= ls1xfb_ioctl,	/* 可用于LCD控制器的Switch Panel位，实现显示单元0和1的相互复制 */
};

#if defined(CONFIG_FB_LS1X_I2C)
static void ls1x_update_var(struct fb_var_screeninfo *var,
			      const struct fb_videomode *modedb)
{
	var->xres = var->xres_virtual = modedb->xres;
	var->yres = modedb->yres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;
	var->xoffset = var->yoffset = 0;
	var->pixclock = modedb->pixclock;
	var->left_margin = modedb->left_margin;
	var->right_margin = modedb->right_margin;
	var->upper_margin = modedb->upper_margin;
	var->lower_margin = modedb->lower_margin;
	var->hsync_len = modedb->hsync_len;
	var->vsync_len = modedb->vsync_len;
	var->sync = modedb->sync;
	var->vmode = modedb->vmode;
}
#endif

static int ls1xfb_init_mode(struct fb_info *info,
			      struct ls1xfb_mach_info *mi)
{
	struct ls1xfb_info *fbi = info->par;
	struct fb_var_screeninfo *var = &info->var;
	int ret = 0;
	u32 total_w, total_h, refresh;
	u64 div_result;
	const struct fb_videomode *m;
//	struct fb_videomode *m;

	/*
	 * Set default value
	 */
	refresh = default_refresh;
	var->xres = default_xres;
	var->yres = default_yres;
	if (default_bpp)
		var->bits_per_pixel = default_bpp;

	/* try to find best video mode. */
	m = fb_find_best_mode(&info->var, &info->modelist);
	if (m) {
//		m->refresh = refresh;
//		m = fb_find_nearest_mode(m, &info->modelist);
//		if (m)
			#if defined(CONFIG_FB_LS1X_I2C)
			ls1x_update_var(&info->var, m);
			#else
			fb_videomode_to_var(&info->var, m);
			#endif
	}
	pr_info("%s:%dx%d-%d@%d\n", m->name, 
		var->xres, var->yres, var->bits_per_pixel, refresh);

	/* Init settings. */
	/* No need for virtual resolution support */
	var->xres_virtual = var->xres;
	var->yres_virtual = info->fix.smem_len /
		(var->xres_virtual * (var->bits_per_pixel >> 3));
	dev_dbg(fbi->dev, "ls1xfb: find best mode: res = %dx%d\n",
				var->xres, var->yres);

	/* correct pixclock. */
	total_w = var->xres + var->left_margin + var->right_margin +
		  var->hsync_len;
	total_h = var->yres + var->upper_margin + var->lower_margin +
		  var->vsync_len;

	div_result = 1000000000000ll;
	do_div(div_result, total_w * total_h * refresh);
	var->pixclock = (u32)div_result;

	return ret;
}

static int ls1xfb_probe(struct platform_device *pdev)
{
	struct ls1xfb_mach_info *mi;
	struct fb_info *info = 0;
	struct ls1xfb_info *fbi = 0;
	struct resource *res;
	struct clk *clk;
	int ret;

	mi = pdev->dev.platform_data;
	if (mi == NULL) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -EINVAL;
	}

	clk = clk_get(&pdev->dev, "pll_clk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "unable to get LCDCLK");
		return PTR_ERR(clk);
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "no IO memory defined\n");
		ret = -ENOENT;
		goto failed_put_clk;
	}

	info = framebuffer_alloc(sizeof(struct ls1xfb_info), &pdev->dev);
	if (info == NULL) {
		ret = -ENOMEM;
		goto failed_put_clk;
	}

	/* Initialize private data */
	fbi = info->par;
	fbi->info = info;
	fbi->clk = clk;
	fbi->dev = info->dev = &pdev->dev;
	fbi->de_mode = mi->de_mode;

	/*
	 * Initialise static fb parameters.
	 */
	info->flags = FBINFO_FLAG_DEFAULT;
	info->node = -1;
	strlcpy(info->fix.id, mi->id, 16);
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	info->fix.type_aux = 0;
	info->fix.xpanstep = 0;
	info->fix.ypanstep = 0;
	info->fix.ywrapstep = 0;
	info->fix.mmio_start = res->start;
	info->fix.mmio_len = res->end - res->start + 1;
	info->fix.accel = FB_ACCEL_NONE;
	info->fbops = &ls1xfb_ops;
	info->pseudo_palette = fbi->pseudo_palette;

	/*
	 * Map LCD controller registers.
	 */
	fbi->reg_base = ioremap_nocache(res->start, resource_size(res));
	if (fbi->reg_base == NULL) {
		ret = -ENOMEM;
		goto failed_free_info;
	}

	/*
	 * Allocate framebuffer memory.
	 */
	if (unlikely(vga_mode)) {
		info->fix.smem_len = PAGE_ALIGN(1920 * 1080 * 4); /* 分配足够的显存，用于切换分辨率 */
	} else {
		info->fix.smem_len = PAGE_ALIGN(default_xres * default_yres * 4);
	}

	info->screen_base = dma_alloc_coherent(fbi->dev, info->fix.smem_len,
						&fbi->fb_start_dma, GFP_KERNEL);
	if (info->screen_base == NULL) {
		ret = -ENOMEM;
		goto failed_free_info;
	}

	info->fix.smem_start = (unsigned long)fbi->fb_start_dma;
	set_graphics_start(info, 0, 0);

	/*
	 * Set video mode according to platform data.
	 */
	#if defined(CONFIG_FB_LS1X_I2C)
	if (ls1xfb_probe_i2c_connector(info, &fbi->edid)) {
		/* 使用自定义modes */
		fb_videomode_to_modelist(mi->modes, mi->num_modes, &info->modelist);
	} else {
		fb_edid_to_monspecs(fbi->edid, &info->monspecs);
		kfree(fbi->edid);
		fb_videomode_to_modelist(info->monspecs.modedb,
					 info->monspecs.modedb_len,
					 &info->modelist);
	}
	#else
	fb_videomode_to_modelist(mi->modes, mi->num_modes, &info->modelist);
	#endif

	/*
	 * init video mode data.
	 */
	ls1xfb_init_mode(info, mi);

	/*
	 * Fill in sane defaults.
	 */
	ret = ls1xfb_check_var(&info->var, info);
	if (ret)
		goto failed_free_fbmem;

	/*
	 * enable controller clock
	 */
//	clk_enable(fbi->clk);

	/* init ls1x lcd controller */
	{
		u32 x;
		int timeout = 204800;
		x = readl(fbi->reg_base + LS1X_FB_CONF);
		writel_reg(x & ~LS1X_FB_CONF_RESET, fbi->reg_base + LS1X_FB_CONF);
//		writel_reg(x & ~LS1X_FB_CONF_OUT_EN, fbi->reg_base + LS1X_FB_CONF);
		x = readl(fbi->reg_base + LS1X_FB_CONF);	/* 不知道为什么这里需要读一次 */
		writel_reg(x & ~LS1X_FB_CONF_RESET, fbi->reg_base + LS1X_FB_CONF);
		x = readl(fbi->reg_base + LS1X_FB_CONF);

		ls1xfb_set_par(info);

		/* 不知道为什么要设置多次才有效,且对LS1X_FB_CONF寄存器的设置不能放在ls1xfb_set_par函数里，
		因为在设置format字段后再设置LS1X_FB_CONF_OUT_EN位会使format字段设置失效 */
		x = readl(fbi->reg_base + LS1X_FB_CONF);
		writel_reg(x | LS1X_FB_CONF_OUT_EN, fbi->reg_base + LS1X_FB_CONF);
		do {
			x = readl(fbi->reg_base + LS1X_FB_CONF);
			writel_reg(x | LS1X_FB_CONF_OUT_EN, fbi->reg_base + LS1X_FB_CONF);
		} while (((x & LS1X_FB_CONF_OUT_EN) == 0)
				&& (timeout-- > 0));
	}

	/*
	 * Allocate color map.
	 */
	if (fb_alloc_cmap(&info->cmap, 256, 0) < 0) {
		ret = -ENOMEM;
		goto failed_free_clk;
	}

	/*
	 * Register framebuffer.
	 */
	ret = register_framebuffer(info);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register ls1x-fb: %d\n", ret);
		ret = -ENXIO;
		goto failed_free_cmap;
	}

	platform_set_drvdata(pdev, fbi);
	return 0;

failed_free_cmap:
	fb_dealloc_cmap(&info->cmap);
failed_free_clk:
//	clk_disable(fbi->clk);
failed_free_fbmem:
	dma_free_coherent(fbi->dev, info->fix.smem_len,
			info->screen_base, fbi->fb_start_dma);
failed_free_info:
	kfree(info);
failed_put_clk:
//	clk_put(clk);

	dev_err(&pdev->dev, "frame buffer device init failed with %d\n", ret);
	return ret;
}

static int ls1xfb_remove(struct platform_device *pdev)
{
	struct ls1xfb_info *fbi = platform_get_drvdata(pdev);
	struct fb_info *info;

	if (!fbi)
		return 0;

	info = fbi->info;

	unregister_framebuffer(info);

	if (info->cmap.len)
		fb_dealloc_cmap(&info->cmap);

	dma_free_coherent(fbi->dev, PAGE_ALIGN(info->fix.smem_len),
				info->screen_base, info->fix.smem_start);

	iounmap(fbi->reg_base);

//	clk_disable(fbi->clk);
//	clk_put(fbi->clk);

	framebuffer_release(info);

	return 0;
}

#ifdef CONFIG_PM

/* suspend and resume support for the lcd controller */
static int ls1xfb_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int ls1xfb_resume(struct platform_device *dev)
{
	return 0;
}

#else
#define ls1xfb_suspend NULL
#define ls1xfb_resume  NULL
#endif

static struct platform_driver ls1xfb_driver = {
	.driver		= {
		.name	= "ls1x-fb",
		.owner	= THIS_MODULE,
	},
	.probe	= ls1xfb_probe,
	.remove	= ls1xfb_remove,
	.suspend	= ls1xfb_suspend,
	.resume	= ls1xfb_resume,
};

static int __init ls1xfb_init(void)
{
	char* options = NULL;
	char *p = NULL, *end = NULL;
	int i;

	fb_get_options(DRIVER_NAME_0, &options);
	if (options == NULL ) {
		fb_get_options(DRIVER_NAME_1, &options);
		if (options == NULL ) {
			fb_get_options(DRIVER_NAME_2, &options);
			if (options == NULL ) {
				fb_get_options(DRIVER_NAME_3, &options);
				vga_mode = 1;
				if (options == NULL ) {
					fb_get_options(DRIVER_NAME_4, &options);
					if (options == NULL ) {
						fb_get_options(DRIVER_NAME_5, &options);
						if (options == NULL ) {
							vga_mode = 0;
						}
					}
				}
			}
		}
	}

	if (options) {
		/* ls1xfb:1920x1080-16@60 */
		for (i=0; i<4; i++)
			if (isdigit(*(options+i)))
				break;	/* 查找options字串中第一个数字时i的值 */
	}
	if (options && (i < 4)) {
		default_xres = simple_strtoul(options+i, &end, 10);
		default_yres = simple_strtoul(end+1, NULL, 10);
		/* depth, bpp default is mi->pix_fmt */
		p = strchr(options, '-');
		if (p) {
			default_bpp = simple_strtoul(p+1, NULL, 0);
		}
		/* refresh */
		p = strchr(options, '@');
		if (p) {
			default_refresh = simple_strtoul(p+1, NULL, 0);
		}
		if ((default_xres<=0 || default_xres>1920) || 
			(default_yres<=0 || default_yres>1080)) {
			pr_info("Warning: Resolution is out of range."
				"MAX resolution is 1920x1080@60Hz\n");
			default_xres = DEFAULT_XRES;
			default_yres = DEFAULT_YRES;
		}
	} else {
		pr_info("Use default resolution %dx%d-%d@%d\n", \
			DEFAULT_XRES, DEFAULT_YRES, DEFAULT_BPP, DEFAULT_REFRESH);
	}

	return platform_driver_register(&ls1xfb_driver);
}
module_init(ls1xfb_init);

static void __exit ls1xfb_exit(void)
{
	platform_driver_unregister(&ls1xfb_driver);
}
module_exit(ls1xfb_exit);

MODULE_AUTHOR("loongson-gz tang <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("Framebuffer driver for the loongson1");
MODULE_LICENSE("GPL");

