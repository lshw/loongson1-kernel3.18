/*
 * i2c-ls1x.h - definitions for the i2c-ls1x interface
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef _LINUX_I2C_LS1X_H
#define _LINUX_I2C_LS1X_H

struct ls1x_i2c_platform_data {
//	u32 reg_shift; /* register offset shift value */
//	u32 reg_io_width; /* register io read/write width */
//	u32 clock_khz; /* input clock in kHz */
	u32 bus_clock_hz; /* i2c bus clock in Hz */
	u8 num_devices; /* number of devices in the devices list */
	struct i2c_board_info const *devices; /* devices connected to the bus */
};

#endif /* _LINUX_I2C_LS1X_H */
