/*
 *  Copyright (c) 2012 Tang, Haifeng <tanghaifeng-gz@loongson.cn> <pengren.mcu@qq.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

/* 自定义fb_videomode */
static struct fb_videomode video_modes[] = {
	{	// HX8238-D控制器
		.name	= "HX8238-D",
		.pixclock	= 139784,
		.refresh	= 80,
		.xres		= 320,
		.yres		= 240,
		.hsync_len	= 32,		// 364-332
		.left_margin	= 68,	// 432-364
		.right_margin	= 12,	// 332-320
		.vsync_len	= 6,		// 254-248
		.upper_margin	= 22,	// 276-254
		.lower_margin	= 8,	// 248-240
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
/*	{	// HX8238-D 国显3.5寸屏
		.name	= "HX8238-D",
		.pixclock	= 153979,
		.refresh	= 60,
		.xres		= 320,
		.yres		= 240,
		.hsync_len	= 2,		// 342-340
		.left_margin	= 68,	// 410-342
		.right_margin	= 20,	// 340-320
		.vsync_len	= 2,		// 264-244
		.upper_margin	= 18,	// 276-246
		.lower_margin	= 4,	// 244-240
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},*/
	{	// ILI9341控制器
		.name	= "ILI9341",
		.pixclock	= 155555,
		.refresh	= 70,
		.xres		= 240,
		.yres		= 320,
		.hsync_len	= 10,
		.left_margin	= 20,
		.right_margin	= 10,
		.vsync_len	= 2,
		.upper_margin	= 2,
		.lower_margin	= 4,
//		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
/*	{	// NT39016D控制器
		.name	= "NT39016D",
		.pixclock	= 155328,
		.refresh	= 60,
		.xres		= 320,
		.yres		= 240,
		.hsync_len	= 1,		// 337-336
		.left_margin	= 71,	// 408-337
		.right_margin	= 16,	// 336-320
		.vsync_len	= 1,		// 251-250
		.upper_margin	= 12,	// 263-251
		.lower_margin	= 10,	// 250-240
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},*/
	{	// NT35310控制器
		.name	= "NT35310",
		.pixclock	= 77470,
		.refresh	= 60,
		.xres		= 320,
		.yres		= 480,
		.hsync_len	= 4,		// 364-360
		.left_margin	= 68,	// 432-364
		.right_margin	= 40,	// 360-320
		.vsync_len	= 2,		// 490-488
		.upper_margin	= 8,	// 498-490
		.lower_margin	= 8,	// 488-480
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
/*	{	//AT043TN13 DE mode
		.name	= "AT043TN13",
		.pixclock	= 111000,
		.refresh	= 60,
		.xres		= 480,
		.yres		= 272,
		.hsync_len	= 41,		// 523-482
		.left_margin	= 2,	// 525-523
		.right_margin	= 2,	// 482-480
		.vsync_len	= 10,		// 284-274
		.upper_margin	= 4,	// 288-284
		.lower_margin	= 2,	// 274-272
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},*/
	{	//LT043A-02AT
		.name	= "LT043A-02AT",
		.pixclock	= 111000,
		.refresh	= 60,
		.xres		= 480,
		.yres		= 272,
		.hsync_len	= 1,		// 489-488
		.left_margin	= 42,	// 531-489
		.right_margin	= 8,	// 488-480
		.vsync_len	= 10,		// 286-276
		.upper_margin	= 2,	// 288-276
		.lower_margin	= 4,	// 276-272
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	// jbt6k74控制器
		.name	= "jbt6k74",
		.pixclock	= 22000,
		.refresh	= 60,
		.xres		= 480,
		.yres		= 640,
		.hsync_len	= 8,		// 496-488
		.left_margin	= 24,	// 520-496
		.right_margin	= 8,	// 488-480
		.vsync_len	= 2,		// 644-642
		.upper_margin	= 4,	// 648-644
		.lower_margin	= 2,	// 642-240
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	//AT056TN52
		.name	= "AT056TN52",
		.pixclock	= 39682,
		.refresh	= 60,
		.xres		= 640,
		.yres		= 480,
		.hsync_len	= 10,		// 666-656
		.left_margin	= 134,	// 800-666
		.right_margin	= 16,	// 656-640
		.vsync_len	= 2,		// 514-512
		.upper_margin	= 11,	// 525-514
		.lower_margin	= 32,	// 512-480
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
/*	{	//AT070TN93
		.name	= "AT070TN93",
		.pixclock	= 32552,
		.refresh	= 60,
		.xres		= 800,
		.yres		= 480,
		.hsync_len	= 80,		// 912-832
		.left_margin	= 112,	// 1024-912
		.right_margin	= 32,	// 832-800
		.vsync_len	= 3,		// 484-481
		.upper_margin	= 16,	// 500-484
		.lower_margin	= 1,	// 481-480
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},*/
	{	//HX8264 HV mode
		.name	= "HX8264",
		.pixclock	= 34209,
		.refresh	= 60,
		.xres		= 800,
		.yres		= 480,
		.hsync_len	= 48,		// 888-840
		.left_margin	= 40,	// 928-888
		.right_margin	= 40,	// 840-800
		.vsync_len	= 3,		// 496-493
		.upper_margin	= 29,	// 525-496
		.lower_margin	= 13,	// 493-480
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	//VESA 800x600@75Hz
		.name	= "VESA",
		.pixclock	= 20202,
		.refresh	= 75,
		.xres		= 800,
		.yres		= 600,
		.hsync_len	= 80,		// 896-816
		.left_margin	= 160,	// 1056-896
		.right_margin	= 16,	// 816-800
		.vsync_len	= 3,		// 484-481
		.upper_margin	= 21,	// 500-484
		.lower_margin	= 1,	// 481-480
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	//VESA 1024x768@60Hz
		.name	= "VESA",
		.pixclock	= 15384,
		.refresh	= 60,
		.xres		= 1024,
		.yres		= 768,
		.hsync_len	= 136,
		.left_margin	= 160,
		.right_margin	= 24,
		.vsync_len	= 6,
		.upper_margin	= 29,
		.lower_margin	= 3,
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	//VESA 1280x1024@75Hz
		.name	= "VESA",
		.pixclock	= 7400,
		.refresh	= 75,
		.xres		= 1280,
		.yres		= 1024,
		.hsync_len	= 144,
		.left_margin	= 248,
		.right_margin	= 16,
		.vsync_len	= 3,
		.upper_margin	= 38,
		.lower_margin	= 1,
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	//VESA 1440x900@75Hz
		.name	= "VESA",
		.pixclock	= 7312,
		.refresh	= 75,
		.xres		= 1440,
		.yres		= 900,
		.hsync_len	= 152,
		.left_margin	= 248,
		.right_margin	= 96,
		.vsync_len	= 6,
		.upper_margin	= 33,
		.lower_margin	= 3,
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
	{	// 1920x1080@60Hz
		.name	= "unknow",
		.pixclock	= 6734,
		.refresh	= 60,
		.xres		= 1920,
		.yres		= 1080,
		.hsync_len	= 44,
		.left_margin	= 148,
		.right_margin	= 88,
		.vsync_len	= 5,
		.upper_margin	= 36,
		.lower_margin	= 4,
		.sync		= FB_SYNC_VERT_HIGH_ACT | FB_SYNC_HOR_HIGH_ACT,
	},
};

#ifdef CONFIG_LOONGSON1_LS1B
struct ls1b_vga ls1b_vga_modes[] = {
	{
		.xres = 800,
		.yres = 600,
		.refresh = 75,
	#if OSC == 25000000
		.ls1b_pll_freq = 0x21813,
		.ls1b_pll_div = 0x8a28ea00,
	#else	//OSC == 33000000
		.ls1b_pll_freq = 0x0080c,
		.ls1b_pll_div = 0x8a28ea00,
	#endif
	},
	{
		.xres = 1024,
		.yres = 768,
		.refresh = 60,
	#if OSC == 25000000
		.ls1b_pll_freq = 0x2181d,
		.ls1b_pll_div = 0x8a28ea00,
	#else	//OSC == 33000000
		.ls1b_pll_freq = 0x21813,
		.ls1b_pll_div = 0x8a28ea00,
	#endif
	},
	{
		.xres = 1280,
		.yres = 1024,
		.refresh = 75,
	#if OSC == 25000000
		.ls1b_pll_freq = 0x3af1e,
		.ls1b_pll_div = 0x8628ea00,
	#else	//OSC == 33000000
		.ls1b_pll_freq = 0x3af14,
		.ls1b_pll_div = 0x8628ea00,
	#endif
	},
	{
		.xres = 1440,
		.yres = 900,
		.refresh = 75,
	#if OSC == 25000000
		.ls1b_pll_freq = 0x3af1f,
		.ls1b_pll_div = 0x8628ea00,
	#else	//OSC == 33000000
		.ls1b_pll_freq = 0x3af14,
		.ls1b_pll_div = 0x8628ea00,
	#endif
	},
	{
		.xres = 1920,
		.yres = 1080,
		.refresh = 60,
	#if OSC == 25000000
		.ls1b_pll_freq = 0x3af23,
		.ls1b_pll_div = 0x86392a00,
	#else	//OSC == 33000000
		.ls1b_pll_freq = 0x3af17,
		.ls1b_pll_div = 0x86392a00,
	#endif
	},
	{},
};
EXPORT_SYMBOL(ls1b_vga_modes);
#endif
