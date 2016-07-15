#ifndef _I8042_LS1X_H
#define _I8042_LS1X_H

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/clk.h>
#include <loongson1.h>

/*
 * Names.
 */

#define I8042_KBD_PHYS_DESC "isa0060/serio0"
#define I8042_AUX_PHYS_DESC "isa0060/serio1"
#define I8042_MUX_PHYS_DESC "isa0060/serio%d"

/*
 * IRQs.
 */

#include <irq.h>
# define I8042_KBD_IRQ	LS1X_KB_IRQ
# define I8042_AUX_IRQ	LS1X_MS_IRQ

/*
 * Register numbers.
 */
#define LS1X_PS2_REG(x) \
		((void __iomem *)KSEG1ADDR(LS1X_PS2_BASE + (x)))

#define I8042_COMMAND_REG		LS1X_PS2_REG(0x4)
#define I8042_STATUS_REG		LS1X_PS2_REG(0x4)
#define I8042_DATA_REG		LS1X_PS2_REG(0x0)

#define DLL_REG		LS1X_PS2_REG(0x8)
#define DLH_REG		LS1X_PS2_REG(0x9)
#define DL_KBD_REG		LS1X_PS2_REG(0xa)
#define DL_AUX_REG		LS1X_PS2_REG(0xb)

static inline int i8042_read_data(void)
{
	return __raw_readb(I8042_DATA_REG);
}

static inline int i8042_read_status(void)
{
	return __raw_readb(I8042_STATUS_REG);
}

static inline void i8042_write_data(int val)
{ 
	__raw_writeb(val, I8042_DATA_REG);
}

static inline void i8042_write_command(int val)
{
	__raw_writeb(val, I8042_COMMAND_REG);
}

static inline int i8042_platform_init(void)
{
/*
 * On some platforms touching the i8042 data register region can do really
 * bad things. Because of this the region is always reserved on such boxes.
 */
 	struct clk *clk;
 	unsigned int div;
 	
 	clk = clk_get(NULL, "apb_clk");
	if (IS_ERR(clk))
		panic("unable to get dc clock, err=%ld", PTR_ERR(clk));
	div = clk_get_rate(clk) / 150000;
	__raw_writeb(div & 0xff, DLL_REG);
	__raw_writeb((div >> 8) & 0xff, DLH_REG);
	__raw_writeb(0xa, DL_KBD_REG);
	__raw_writeb(0xa, DL_AUX_REG);

	i8042_reset = 1;

	return 0;
}

static inline void i8042_platform_exit(void)
{
}

#endif /* _I8042_IO_H */
