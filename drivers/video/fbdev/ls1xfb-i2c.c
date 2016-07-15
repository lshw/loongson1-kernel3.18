/*
 * Copyright (c) 2015 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
  *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <video/ls1xfb.h>
#include "edid.h"

#define DDC_ADDR	0x50

static int ls1x_ddc_read(struct i2c_adapter *adapter,
		unsigned char *buf, u16 count, u8 offset)
{
	int r, retries;

	for (retries = 3; retries > 0; retries--) {
		struct i2c_msg msgs[] = {
			{
				.addr   = DDC_ADDR,
				.flags  = 0,
				.len    = 1,
				.buf    = &offset,
			}, {
				.addr   = DDC_ADDR,
				.flags  = I2C_M_RD,
				.len    = count,
				.buf    = buf,
			}
		};

		r = i2c_transfer(adapter, msgs, 2);
		if (r == 2)
			return 0;

		if (r != -EAGAIN)
			break;
	}

	return r < 0 ? r : -EIO;
}

static int ls1x_read_edid(struct i2c_adapter *i2c_adapter,
		u8 *edid, int len)
{
	int r, l, bytes_read;

	l = min(EDID_LENGTH, len);
	r = ls1x_ddc_read(i2c_adapter, edid, l, 0);
	if (r)
		return r;

	bytes_read = l;

	/* if there are extensions, read second block */
	if (len > EDID_LENGTH && edid[0x7e] > 0) {
		l = min(EDID_LENGTH, len - EDID_LENGTH);

		r = ls1x_ddc_read(i2c_adapter, edid + EDID_LENGTH,
				l, EDID_LENGTH);
		if (r)
			return r;

		bytes_read += l;
	}

	return bytes_read;
}

int ls1xfb_probe_i2c_connector(struct fb_info *info, u8 **out_edid)
{
	struct ls1xfb_info *fbi = info->par;
	struct ls1xfb_mach_info *mi = fbi->dev->platform_data;
	int i2c_bus_num, r;
	unsigned char *edid = kmalloc(EDID_LENGTH, GFP_KERNEL);

	i2c_bus_num = mi->i2c_bus_num;

	if (i2c_bus_num != -1) {
		struct i2c_adapter *adapter;

		adapter = i2c_get_adapter(i2c_bus_num);
		if (!adapter) {
			dev_err(fbi->dev,
					"Failed to get I2C adapter, bus %d\n",
					i2c_bus_num);
			return -ENODEV;
		}

		fbi->i2c_adapter = adapter;
	}

	r = ls1x_ddc_read(fbi->i2c_adapter, edid, 1, 0);
	if (r == 0) {
		ls1x_read_edid(fbi->i2c_adapter, edid, EDID_LENGTH);
	} else {
		edid = NULL;
	}

	if (!edid) {
		/* try to get from firmware */
		const u8 *e = fb_firmware_edid(info->device);

		if (e)
			edid = kmemdup(e, EDID_LENGTH, GFP_KERNEL);
	}

	*out_edid = edid;

	return (edid) ? 0 : 1;
}

MODULE_LICENSE("GPL");
