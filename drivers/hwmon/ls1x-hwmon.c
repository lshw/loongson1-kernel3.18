/*
 *  Copyright (c) 2014 Haifeng Tang <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License. See the file COPYING in the main directory of this archive for
 *  more details.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#include <loongson1.h>

#include <hwmon.h>

struct ls1x_hwmon_attr {
	struct sensor_device_attribute	in;
	struct sensor_device_attribute	label;
	char				in_name[12];
	char				label_name[12];
};

/**
 * struct ls1x_hwmon - ADC hwmon client information
 * @lock: Access lock to serialise the conversions.
 * @client: The client we registered with the LS1X ADC core.
 * @hwmon_dev: The hwmon device we created.
 * @attr: The holders for the channel attributes.
*/
struct ls1x_hwmon {
	struct mutex		lock;
	struct device		*hwmon_dev;

	struct ls1x_hwmon_attr	attrs[4];

	struct clk		*clk;
	void __iomem		*regs;
	int			 irq;

	int single;
	unsigned int adc_data_old;
};

static void ls1x_adc_hwinit(struct ls1x_hwmon *hwmon)
{
	unsigned int x;

	__raw_writel(__raw_readl(LS1X_MUX_CTRL0) & (~ADC_SHUT), LS1X_MUX_CTRL0);

	/* reset */
//	writel(0x20, hwmon->regs + ADC_S_CTRL);
//	while (readl(hwmon->regs + ADC_S_CTRL) & 0x20) {
//	}

	/* powerdown */
	x = readl(hwmon->regs + ADC_S_CTRL);
	writel(x | 0x40, hwmon->regs + ADC_S_CTRL);

	writel(0x00, hwmon->regs + ADC_C_CTRL);
	writel(0x1f, hwmon->regs + ADC_INT);
//	writel(0x02f001ff, hwmon->regs + ADC_CNT);
}

static unsigned int ls1x_adc_read(struct ls1x_hwmon *hwmon, unsigned int ch)
{
	unsigned int x, i;
	unsigned int adc_data = 0;
	unsigned int timeout = 9000000;

	/* powerup */
	writel(0x00, hwmon->regs + ADC_S_CTRL);

	if (hwmon->single) {
		dev_dbg(hwmon->hwmon_dev, "reading channel %d single\n", ch);
		x = readl(hwmon->regs + ADC_S_CTRL);
		x &= (~0xf);
		x |= (0x1 << ch);
		writel(x, hwmon->regs + ADC_S_CTRL);
		writel(x | 0x10, hwmon->regs + ADC_S_CTRL);
//		while (readl(hwmon->regs + ADC_S_CTRL) & 0x10) {
//		}
		switch (ch) {
		case 0:
			while ((readl(hwmon->regs + ADC_S_DOUT0) & 0x80000000) && (timeout-- > 0)) {
			}
			adc_data = readl(hwmon->regs + ADC_S_DOUT0) & 0x3ff;
			break;
		case 1:
			while ((readl(hwmon->regs + ADC_S_DOUT0) & 0x80000000) && (timeout-- > 0)) {
			}
			adc_data = (readl(hwmon->regs + ADC_S_DOUT0) >> 16) & 0x3ff;
			break;
		case 2:
			while ((readl(hwmon->regs + ADC_S_DOUT1) & 0x80000000) && (timeout-- > 0)) {
			}
			adc_data = readl(hwmon->regs + ADC_S_DOUT1) & 0x3ff;
			break;
		case 3:
			while ((readl(hwmon->regs + ADC_S_DOUT1) & 0x80000000) && (timeout-- > 0)) {
			}
			adc_data = (readl(hwmon->regs + ADC_S_DOUT1) >> 16) & 0x3ff;
			break;
		}
		writel(0x00, hwmon->regs + ADC_S_CTRL);	/* stop */
		/* 1c300A 使用单次转换时会出现死等的问题，必需执行复位，1C300B则不会 */
	#ifndef CONFIG_LS1CV2_MACH
		/* reset */
		writel(0x20, hwmon->regs + ADC_S_CTRL);
		while (readl(hwmon->regs + ADC_S_CTRL) & 0x20) {
		}
	#endif
		/* powerdown */
		x = readl(hwmon->regs + ADC_S_CTRL);
		writel(x | 0x40, hwmon->regs + ADC_S_CTRL);
	} else {
		dev_dbg(hwmon->hwmon_dev, "reading channel %d continually\n", ch);

		writel(0x11f, hwmon->regs + ADC_INT);

		x = readl(hwmon->regs + ADC_C_CTRL);
		x &= (~0xf);
		x |= (0x1 << ch);
		writel(x | 0x80, hwmon->regs + ADC_C_CTRL);

		x = readl(hwmon->regs + ADC_S_CTRL);
		x &= (~0xf);
		x |= (0x1 << ch);
		writel(x, hwmon->regs + ADC_S_CTRL);
		writel(x | 0x10, hwmon->regs + ADC_S_CTRL);

		while (timeout) {
			if (readl(hwmon->regs + ADC_INT) & 0x10) {
				break;
			}
			timeout--;
		}
		adc_data = readl(hwmon->regs + ADC_C_DOUT) & 0x3ff;

		x = readl(hwmon->regs + ADC_C_CTRL);
		writel(x & (~0x80), hwmon->regs + ADC_C_CTRL);	/* stop */
	}

	if (!timeout) {
		adc_data = hwmon->adc_data_old;
		printk("%s: timeout\n", __func__);
	} else {
		hwmon->adc_data_old = adc_data;
	}
	/* clean int */
	writel(0x1f, hwmon->regs + ADC_INT);

	return adc_data;
}

/**
 * ls1x_hwmon_read_ch - read a value from a given adc channel.
 * @dev: The device.
 * @hwmon: Our state.
 * @channel: The channel we're reading from.
 *
 * Read a value from the @channel with the proper locking and sleep until
 * either the read completes or we timeout awaiting the ADC core to get
 * back to us.
 */
static int ls1x_hwmon_read_ch(struct device *dev,
			     struct ls1x_hwmon *hwmon, int channel)
{
	int ret;

	ret = mutex_lock_interruptible(&hwmon->lock);
	if (ret < 0)
		return ret;

	dev_dbg(dev, "reading channel %d\n", channel);

	ret = ls1x_adc_read(hwmon, channel);
	mutex_unlock(&hwmon->lock);

	return ret;
}

#ifdef CONFIG_SENSORS_LS1X_RAW
/**
 * ls1x_hwmon_show_raw - show a conversion from the raw channel number.
 * @dev: The device that the attribute belongs to.
 * @attr: The attribute being read.
 * @buf: The result buffer.
 *
 * This show deals with the raw attribute, registered for each possible
 * ADC channel. This does a conversion and returns the raw (un-scaled)
 * value returned from the hardware.
 */
static ssize_t ls1x_hwmon_show_raw(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct ls1x_hwmon *adc = platform_get_drvdata(to_platform_device(dev));
	struct sensor_device_attribute *sa = to_sensor_dev_attr(attr);
	struct ls1x_hwmon_pdata *pdata = dev->platform_data;
	struct ls1x_hwmon_chcfg *cfg;
	int ret;

	cfg = pdata->in[sa->index];
	if (cfg) {
		adc->single = cfg->single;
	} else {
		adc->single = 0;
	}

	ret = ls1x_hwmon_read_ch(dev, adc, sa->index);

	return  (ret < 0) ? ret : snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

#define DEF_ADC_ATTR(x)	\
	static SENSOR_DEVICE_ATTR(adc##x##_raw, S_IRUGO, ls1x_hwmon_show_raw, NULL, x)

DEF_ADC_ATTR(0);
DEF_ADC_ATTR(1);
DEF_ADC_ATTR(2);
DEF_ADC_ATTR(3);

static struct attribute *ls1x_hwmon_attrs[5] = {
	&sensor_dev_attr_adc0_raw.dev_attr.attr,
	&sensor_dev_attr_adc1_raw.dev_attr.attr,
	&sensor_dev_attr_adc2_raw.dev_attr.attr,
	&sensor_dev_attr_adc3_raw.dev_attr.attr,
	NULL,
};

static struct attribute_group ls1x_hwmon_attrgroup = {
	.attrs	= ls1x_hwmon_attrs,
};

static inline int ls1x_hwmon_add_raw(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &ls1x_hwmon_attrgroup);
}

static inline void ls1x_hwmon_remove_raw(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &ls1x_hwmon_attrgroup);
}

#else

static inline int ls1x_hwmon_add_raw(struct device *dev) { return 0; }
static inline void ls1x_hwmon_remove_raw(struct device *dev) { }

#endif /* CONFIG_SENSORS_LS1X_RAW */

/**
 * ls1x_hwmon_ch_show - show value of a given channel
 * @dev: The device that the attribute belongs to.
 * @attr: The attribute being read.
 * @buf: The result buffer.
 *
 * Read a value from the ADC and scale it before returning it to the
 * caller. The scale factor is gained from the channel configuration
 * passed via the platform data when the device was registered.
 */
static ssize_t ls1x_hwmon_ch_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct sensor_device_attribute *sen_attr = to_sensor_dev_attr(attr);
	struct ls1x_hwmon *hwmon = platform_get_drvdata(to_platform_device(dev));
	struct ls1x_hwmon_pdata *pdata = dev->platform_data;
	struct ls1x_hwmon_chcfg *cfg;
	int ret;

	cfg = pdata->in[sen_attr->index];
	if (cfg) {
		hwmon->single = cfg->single;
	} else {
		hwmon->single = 0;
	}

	ret = ls1x_hwmon_read_ch(dev, hwmon, sen_attr->index);
	if (ret < 0)
		return ret;

	ret *= cfg->mult;
	ret = DIV_ROUND_CLOSEST(ret, cfg->div);

	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

/**
 * ls1x_hwmon_label_show - show label name of the given channel.
 * @dev: The device that the attribute belongs to.
 * @attr: The attribute being read.
 * @buf: The result buffer.
 *
 * Return the label name of a given channel
 */
static ssize_t ls1x_hwmon_label_show(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct sensor_device_attribute *sen_attr = to_sensor_dev_attr(attr);
	struct ls1x_hwmon_pdata *pdata = dev->platform_data;
	struct ls1x_hwmon_chcfg *cfg;

	cfg = pdata->in[sen_attr->index];

	return snprintf(buf, PAGE_SIZE, "%s\n", cfg->name);
}

/**
 * ls1x_hwmon_create_attr - create hwmon attribute for given channel.
 * @dev: The device to create the attribute on.
 * @cfg: The channel configuration passed from the platform data.
 * @channel: The ADC channel number to process.
 *
 * Create the scaled attribute for use with hwmon from the specified
 * platform data in @pdata. The sysfs entry is handled by the routine
 * ls1x_hwmon_ch_show().
 *
 * The attribute name is taken from the configuration data if present
 * otherwise the name is taken by concatenating in_ with the channel
 * number.
 */
static int ls1x_hwmon_create_attr(struct device *dev,
				 struct ls1x_hwmon_chcfg *cfg,
				 struct ls1x_hwmon_attr *attrs,
				 int channel)
{
	struct sensor_device_attribute *attr;
	int ret;

	snprintf(attrs->in_name, sizeof(attrs->in_name), "in%d_input", channel);

	attr = &attrs->in;
	attr->index = channel;
	sysfs_attr_init(&attr->dev_attr.attr);
	attr->dev_attr.attr.name  = attrs->in_name;
	attr->dev_attr.attr.mode  = S_IRUGO;
	attr->dev_attr.show = ls1x_hwmon_ch_show;

	ret =  device_create_file(dev, &attr->dev_attr);
	if (ret < 0) {
		dev_err(dev, "failed to create input attribute\n");
		return ret;
	}

	/* if this has a name, add a label */
	if (cfg->name) {
		snprintf(attrs->label_name, sizeof(attrs->label_name),
			 "in%d_label", channel);

		attr = &attrs->label;
		attr->index = channel;
		sysfs_attr_init(&attr->dev_attr.attr);
		attr->dev_attr.attr.name  = attrs->label_name;
		attr->dev_attr.attr.mode  = S_IRUGO;
		attr->dev_attr.show = ls1x_hwmon_label_show;

		ret = device_create_file(dev, &attr->dev_attr);
		if (ret < 0) {
			device_remove_file(dev, &attrs->in.dev_attr);
			dev_err(dev, "failed to create label attribute\n");
		}
	}

	return ret;
}

static void ls1x_hwmon_remove_attr(struct device *dev,
				  struct ls1x_hwmon_attr *attrs)
{
	device_remove_file(dev, &attrs->in.dev_attr);
	device_remove_file(dev, &attrs->label.dev_attr);
}

/**
 * ls1x_hwmon_probe - device probe entry.
 * @dev: The device being probed.
*/
static int ls1x_hwmon_probe(struct platform_device *dev)
{
	struct ls1x_hwmon_pdata *pdata = dev->dev.platform_data;
	struct ls1x_hwmon *hwmon;
	struct resource *regs;
	int ret = 0;
	int i;

	if (!pdata) {
		dev_err(&dev->dev, "no platform data supplied\n");
		return -EINVAL;
	}

	hwmon = kzalloc(sizeof(struct ls1x_hwmon), GFP_KERNEL);
	if (hwmon == NULL) {
		dev_err(&dev->dev, "no memory\n");
		return -ENOMEM;
	}

/*	hwmon->irq = platform_get_irq(dev, 1);
	if (hwmon->irq <= 0) {
		dev_err(dev->dev, "failed to get hwmon irq\n");
		ret = -ENOENT;
		goto err_alloc;
	}

	ret = request_irq(hwmon->irq, ls1x_hwmon_irq, 0, dev_name(dev), hwmon);
	if (ret < 0) {
		dev_err(dev->dev, "failed to attach hwmon irq\n");
		goto err_alloc;
	}*/

	hwmon->clk = clk_get(&dev->dev, "apb_clk");
	if (IS_ERR(hwmon->clk)) {
		dev_err(&dev->dev, "failed to get hwmon clock\n");
		ret = PTR_ERR(hwmon->clk);
		goto err_irq;
	}

	regs = platform_get_resource(dev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(&dev->dev, "failed to find registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	hwmon->regs = ioremap(regs->start, resource_size(regs));
	if (!hwmon->regs) {
		dev_err(&dev->dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	platform_set_drvdata(dev, hwmon);

	mutex_init(&hwmon->lock);

	/* Initialize the ADC controller. */
	ls1x_adc_hwinit(hwmon);

	/* add attributes for our adc devices. */
	ret = ls1x_hwmon_add_raw(&dev->dev);
	if (ret)
		goto err_clk;

	/* register with the hwmon core */
	hwmon->hwmon_dev = hwmon_device_register(&dev->dev);
	if (IS_ERR(hwmon->hwmon_dev)) {
		dev_err(&dev->dev, "error registering with hwmon\n");
		ret = PTR_ERR(hwmon->hwmon_dev);
		goto err_raw_attribute;
	}

	for (i = 0; i < ARRAY_SIZE(pdata->in); i++) {
		struct ls1x_hwmon_chcfg *cfg = pdata->in[i];

		if (!cfg)
			continue;

		if (cfg->mult >= 0x10000)
			dev_warn(&dev->dev, "channel %d multiplier too large\n", i);

		if (cfg->div == 0) {
			dev_err(&dev->dev, "channel %d divider zero\n", i);
			continue;
		}

		ret = ls1x_hwmon_create_attr(&dev->dev, pdata->in[i],
					    &hwmon->attrs[i], i);
		if (ret) {
			dev_err(&dev->dev, "error creating channel %d\n", i);
			for (i--; i >= 0; i--)
				ls1x_hwmon_remove_attr(&dev->dev, &hwmon->attrs[i]);
			goto err_hwmon_register;
		}
	}

	return 0;

err_hwmon_register:
	hwmon_device_unregister(hwmon->hwmon_dev);

err_raw_attribute:
	ls1x_hwmon_remove_raw(&dev->dev);

err_clk:
	clk_put(hwmon->clk);

err_irq:
//	free_irq(hwmon->irq, adc);

//err_mem:
	kfree(hwmon);
	return ret;
}

static int ls1x_hwmon_remove(struct platform_device *dev)
{
	struct ls1x_hwmon *hwmon = platform_get_drvdata(dev);
	int i;

	ls1x_hwmon_remove_raw(&dev->dev);

	for (i = 0; i < ARRAY_SIZE(hwmon->attrs); i++)
		ls1x_hwmon_remove_attr(&dev->dev, &hwmon->attrs[i]);

	hwmon_device_unregister(hwmon->hwmon_dev);

	iounmap(hwmon->regs);
//	free_irq(hwmon->irq, hwmon);
//	clk_disable(hwmon->clk);
	clk_put(hwmon->clk);

	return 0;
}

#ifdef CONFIG_PM
static int ls1x_hwmon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ls1x_hwmon *hwmon = platform_get_drvdata(pdev);
	u32 x;

	__raw_writel(__raw_readl(LS1X_MUX_CTRL0) | ADC_SHUT, LS1X_MUX_CTRL0);
	/* powerdown */
	x = readl(hwmon->regs + ADC_S_CTRL);
	writel(x | 0x40, hwmon->regs + ADC_S_CTRL);

//	disable_irq(hwmon->irq);
//	clk_disable(hwmon->clk);

	return 0;
}

static int ls1x_hwmon_resume(struct platform_device *pdev)
{
//	struct ls1x_hwmon *hwmon = platform_get_drvdata(pdev);

//	clk_enable(hwmon->clk);
//	enable_irq(hwmon->irq);

	__raw_writel(__raw_readl(LS1X_MUX_CTRL0) & (~ADC_SHUT), LS1X_MUX_CTRL0);

	return 0;
}

#else
#define ls1x_hwmon_suspend NULL
#define ls1x_hwmon_resume NULL
#endif

static struct platform_driver ls1x_hwmon_driver = {
	.driver	= {
		.name		= "ls1x-hwmon",
		.owner		= THIS_MODULE,
	},
	.probe		= ls1x_hwmon_probe,
	.remove		= ls1x_hwmon_remove,
	.suspend	= ls1x_hwmon_suspend,
	.resume		= ls1x_hwmon_resume,
};

static int __init ls1x_hwmon_init(void)
{
	return platform_driver_register(&ls1x_hwmon_driver);
}

static void __exit ls1x_hwmon_exit(void)
{
	platform_driver_unregister(&ls1x_hwmon_driver);
}

module_init(ls1x_hwmon_init);
module_exit(ls1x_hwmon_exit);

MODULE_AUTHOR("Haifeng Tang <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("LS1C ADC HWMon driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:ls1x-hwmon");
