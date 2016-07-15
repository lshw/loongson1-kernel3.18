/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <sound/ac97_codec.h>
#include <sound/ls1x-lib.h>

#include "ls1x-ac97-lib.h"

static DEFINE_MUTEX(car_mutex);

struct ls1x_audio {
	void __iomem *base;
	int irq;
	int no_vra;
};

static struct ls1x_audio ls1x_audio_state;

unsigned short ls1x_ac97_read(struct snd_ac97 *ac97, unsigned short reg)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	unsigned short val = -1;
	int i = 1000;
	u32 data = 0;

	mutex_lock(&car_mutex);

	data |= CODEC_WR;
	data |= ((u32)reg << CODEC_ADR_OFFSET);
	writel(data, state->base + CRAC);

	/* now wait for the data */
	while (i-- > 0) {
		if ((readl(state->base + INTRAW) & CR_DONE) != 0)
			break;
		udelay(500);
	}
	if (i > 0) {
		readl(state->base + INT_CRCLR);
		val = readl(state->base + CRAC) & 0xffff;
	}
	else {
		printk("AC97 command read timeout\n");
	}

	mutex_unlock(&car_mutex);

	return val;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_read);

void ls1x_ac97_write(struct snd_ac97 *ac97, unsigned short reg,
			unsigned short val)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	int i = 1000;
	u32 data = 0;

	mutex_lock(&car_mutex);

	data &= ~(CODEC_WR);
	data |= ((u32)reg << CODEC_ADR_OFFSET) | ((u32)val << CODEC_DAT_OFFSET);
	writel(data, state->base + CRAC);

	while (i-- > 0) {
		if ((readl(state->base + INTRAW) & CW_DONE) != 0)
			break;
		udelay(500);
	}
	if (i > 0) {
		readl(state->base + INT_CWCLR);
	}
	else {
		printk("AC97 command write timeout\n");
	}

	mutex_unlock(&car_mutex);
}
EXPORT_SYMBOL_GPL(ls1x_ac97_write);

bool ls1x_ac97_try_warm_reset(struct snd_ac97 *ac97)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	u32 x, timeout = 5;

	x = readl(state->base + CSR);

	do {
		writel(x | 0x01, state->base + CSR);
		mdelay(60);
		x = readl(state->base + CSR);
		timeout--;
	} while ((x & 0x02) && timeout);

	writel(x & 0xfe, state->base + CSR);

	return true;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_try_warm_reset);

bool ls1x_ac97_try_cold_reset(struct snd_ac97 *ac97)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	u32 x, timeout = 5;

	x = readl(state->base + CSR);

	do {
		writel(x | 0x01, state->base + CSR);
		mdelay(60);
		x = readl(state->base + CSR);
		timeout--;
	} while ((x & 0x02) && timeout);

	writel(x & 0xfe, state->base + CSR);

	return true;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_try_cold_reset);

void ls1x_ac97_channel_config_out(uint32_t conf)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	uint32_t config = conf;
	u32 x = 0;

	if (state->no_vra) {
		config &= (~(0x1 << 1));
	} else {
		config |= (0x1 << 1);
	}

	config |= 0x41;	/* enable dma and channel */
	config |= (0x3 << 4);	/* fifo */

	x = readl(state->base + OCC0);
	x &= 0xffff0000;
	x = x | (config << 8) | config;

	writel(x, state->base + OCC0);
}
EXPORT_SYMBOL_GPL(ls1x_ac97_channel_config_out);

void ls1x_ac97_channel_config_in(uint32_t conf)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	uint32_t config = conf;
	u32 x = 0;

	if (state->no_vra) {
		config &= (~(0x1 << 1));
	} else {
		config |= (0x1 << 1);
	}

	config |= 0x41;	/* enable dma and channel */
	config |= (0x3 << 4);	/* fifo */

//	x = readl(state->base + ICC);
	x = x | (config << 24) | (config << 16) | (config << 8) | config;

	writel(x, state->base + ICC);
}
EXPORT_SYMBOL_GPL(ls1x_ac97_channel_config_in);

static irqreturn_t ls1x_ac97_irq(int irq, void *dev_id)
{
	return IRQ_NONE;
}

#ifdef CONFIG_PM
int ls1x_ac97_hw_suspend(void)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	u32 x;

	x = readl(state->base + OCC0);
	x &= 0xbebebebe;
	writel(x, state->base + OCC0);
	x = readl(state->base + OCC1);
	x &= 0xbebebebe;
	writel(x, state->base + OCC1);
	x = readl(state->base + OCC2);
	x &= 0xbebebebe;
	writel(x, state->base + OCC2);
	x = readl(state->base + ICC);
	x &= 0xbebebebe;
	writel(x, state->base + ICC);

	x = readl(state->base + CSR);
	writel(x | 0x02, state->base + CSR);

	return 0;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_hw_suspend);

int ls1x_ac97_hw_resume(void)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	u32 x;

	x = readl(state->base + OCC0);
	x |= 0x41414141;
	writel(x, state->base + OCC0);
	x = readl(state->base + OCC1);
	x |= 0x41414141;
	writel(x, state->base + OCC1);
	x = readl(state->base + OCC2);
	x |= 0x41414141;
	writel(x, state->base + OCC2);
	x = readl(state->base + ICC);
	x |= 0x41414141;
	writel(x, state->base + ICC);

	x = readl(state->base + CSR);
	writel(x & (~0x02), state->base + CSR);

	return 0;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_hw_resume);
#endif

int ls1x_ac97_hw_probe(struct platform_device *pdev)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	struct resource *res;
	int ret;

	res =  platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		return -ENOENT;
	}

	if (!request_mem_region(res->start, resource_size(res), "ls1x-audio"))
		return -EBUSY;

	state->base = ioremap(res->start, resource_size(res));
	if (!state->base) {
		dev_err(&pdev->dev, "ls1x-audio - failed to map controller\n");
		ret = -ENOMEM;
		goto out1;
	}

	state->irq = platform_get_irq(pdev, 0);
	if (!state->irq) {
		ret = -ENXIO;
		goto out2;
	}

	/* reset ls1x ac97 controller */
	writel(0x00, state->base + CSR);
	ls1x_ac97_try_cold_reset(NULL);
	/* config channels */
	writel(0x30303030, state->base + OCC0);
	writel(0x30303030, state->base + OCC1);
	writel(0x30303030, state->base + OCC2);
	writel(0x30303030, state->base + ICC);
	/* disable irqreturn */
	readl(state->base + INT_CLR);
	readl(state->base + INT_OCCLR);
	readl(state->base + INT_ICCLR);
	readl(state->base + INT_CWCLR);
	readl(state->base + INT_CRCLR);
//	writel(0xffffffff, state->base + INTM);
	writel(0x00000000, state->base + INTRAW);
	writel(0x00000000, state->base + INTM);

	if (ls1x_ac97_read(NULL, AC97_EXTENDED_ID) & 0x01) {
		state->no_vra = 0;
	} else {
		state->no_vra = 1;
	}

	ret = request_irq(state->irq, ls1x_ac97_irq, IRQF_DISABLED, pdev->name, NULL);
	if (ret < 0)
		goto out2;

	return 0;

out2:
	iounmap(state->base);
out1:
	release_mem_region(res->start, resource_size(res));
	return ret;
}
EXPORT_SYMBOL_GPL(ls1x_ac97_hw_probe);

void ls1x_ac97_hw_remove(struct platform_device *pdev)
{
	struct ls1x_audio *state = &ls1x_audio_state;
	struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	free_irq(state->irq, NULL);
	iounmap(state->base);
	release_mem_region(res->start, resource_size(res));
}
EXPORT_SYMBOL_GPL(ls1x_ac97_hw_remove);

MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("loongson1 sound library");
MODULE_LICENSE("GPL");
