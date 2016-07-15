/*
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#include <linux/clk.h>
#include <linux/delay.h>

#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>

#include <sound/ls1x-lib.h>

#include <loongson1.h>
#include "ls1x-i2s.h"

struct ls1x_i2s {
	struct resource *mem;
	void __iomem *base;
	dma_addr_t phys_base;

	struct clk *clk_i2s;
};

static struct ls1x_pcm_dma_params ls1x_i2s_pcm_stereo_out = {
	.name = "I2S PCM Stereo out",
	.dev_addr = 0x1fe60010,
};

static struct ls1x_pcm_dma_params ls1x_i2s_pcm_stereo_in = {
	.name = "I2S PCM Stereo in",
	.dev_addr = 0x1fe6000c,
};

static int ls1x_i2s_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (dai->active)
		return 0;

	writel(readl(LS1X_MUX_CTRL0) & (~I2S_SHUT), LS1X_MUX_CTRL0);

	return 0;
}

static void ls1x_i2s_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	if (dai->active)
		return;

	writel(readl(LS1X_MUX_CTRL0) | I2S_SHUT, LS1X_MUX_CTRL0);
}

static int ls1x_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			      struct snd_soc_dai *dai)
{
	struct ls1x_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	uint32_t ctrl;
	uint32_t mask;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mask = CONTROL_TX_EN | CONTROL_TX_DMA_EN;
//		mask = CONTROL_TX_EN | CONTROL_TX_DMA_EN | CONTROL_TX_INT_EN;
	else
		mask = CONTROL_RX_EN | CONTROL_RX_DMA_EN;
//		mask = CONTROL_RX_EN | CONTROL_RX_DMA_EN | CONTROL_RX_INT_EN;

	ctrl = readl(i2s->base + LS1X_IIS_CONTROL);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		ctrl |= mask;
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		ctrl &= ~mask;
		break;
	default:
		return -EINVAL;
	}

	writel(ctrl, i2s->base + LS1X_IIS_CONTROL);

	return 0;
}

static int ls1x_i2s_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct ls1x_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	uint32_t ctrl;

	ctrl = readl(i2s->base + LS1X_IIS_CONTROL);

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:	/* enable I2S mode */
		break;
	case SND_SOC_DAIFMT_MSB:
//		ctrl |= CONTROL_MSB_LSB;
		break;
	case SND_SOC_DAIFMT_LSB:
//		ctrl &= ~CONTROL_MSB_LSB;	/* LSB (right-) justified */
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:	/* CODEC master */
		ctrl &= ~CONTROL_MASTER;	/* LS1X I2S slave mode */
		break;
	case SND_SOC_DAIFMT_CBS_CFS:	/* CODEC slave */
		ctrl |= CONTROL_MASTER;	/* LS1X I2S Master mode */
		break;
	default:
		return -EINVAL;
	}

	writel(ctrl, i2s->base + LS1X_IIS_CONTROL);

	return 0;
}

static int ls1x_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct ls1x_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	struct ls1x_pcm_dma_params *dma_data;
	unsigned int sample_size;
	unsigned int i2s_clk;
	unsigned char bck_ratio;

	uint32_t ctrl;
	uint32_t conf;

	ctrl = readl(i2s->base + LS1X_IIS_CONTROL);
	conf = readl(i2s->base + LS1X_IIS_CONFIG);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_U8:
		sample_size = 8;
		ctrl |= CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_U16_LE:
		sample_size = 16;
		ctrl |= CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S16_BE:
	case SNDRV_PCM_FORMAT_U16_BE:
		sample_size = 16;
		ctrl &= ~CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_U24_LE:
		sample_size = 24;
		ctrl |= CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S24_BE:
	case SNDRV_PCM_FORMAT_U24_BE:
		sample_size = 24;
		ctrl &= ~CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_U32_LE:
		sample_size = 32;
		ctrl |= CONTROL_MSB_LSB;
		break;
	case SNDRV_PCM_FORMAT_S32_BE:
	case SNDRV_PCM_FORMAT_U32_BE:
		sample_size = 32;
		ctrl &= ~CONTROL_MSB_LSB;
		break;
	default:
		printk("\nerror SNDRV_PCM_FORMAT=%d\n", params_format(params));
		return -EINVAL;
	}

	/* BCK的频率=2×采样频率×采样位数 */
	/* 帧时钟LRCK，用于切换左右声道的数据,LRCK的频率等于采样频率 */
	i2s_clk = clk_get_rate(i2s->clk_i2s) / 2;
	bck_ratio = (i2s_clk / (params_rate(params) * sample_size * 2)) - 1;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		dma_data = &ls1x_i2s_pcm_stereo_out;
	}
	else {
		dma_data = &ls1x_i2s_pcm_stereo_in;
	}

	conf &= 0x000000ff;
	conf |= (sample_size<<24) | (sample_size<<16) | (bck_ratio<<8);
	writel(conf, i2s->base + LS1X_IIS_CONFIG);
	writel(ctrl, i2s->base + LS1X_IIS_CONTROL);

	snd_soc_dai_set_dma_data(dai, substream, dma_data);

	return 0;
}

static int ls1x_i2s_set_sysclk(struct snd_soc_dai *dai, int clk_id,
	unsigned int freq, int dir)
{
	struct ls1x_i2s *i2s = snd_soc_dai_get_drvdata(dai);
	unsigned int i2s_clk;
	unsigned char sck_ratio;

	uint32_t conf;

	conf = readl(i2s->base + LS1X_IIS_CONFIG);
	conf &= 0xffffff00;

	/* 有时为了使系统间能够更好地同步，
	   还需要另外传输一个信号MCLK，称为主时钟，也叫系统时钟（Sys Clock），
	   是采样频率的256倍或384倍。 */
	i2s_clk = clk_get_rate(i2s->clk_i2s) / 2;
	sck_ratio = (i2s_clk / freq) - 1;
	conf |= sck_ratio;
	writel(conf, i2s->base + LS1X_IIS_CONFIG);

	return 0;
}

#ifdef CONFIG_PM
static int ls1x_i2s_dai_suspend(struct snd_soc_dai *dai)
{
	return 0;
}

static int ls1x_i2s_dai_resume(struct snd_soc_dai *dai)
{
	return 0;
}

#else
#define ls1x_i2s_dai_suspend	NULL
#define ls1x_i2s_dai_resume	NULL
#endif

static int ls1x_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct ls1x_i2s *i2s = snd_soc_dai_get_drvdata(dai);

	writel(CONTROL_MASTER | CONTROL_MSB_LSB, i2s->base + LS1X_IIS_CONTROL);
	writel(0x0000, i2s->base + LS1X_IIS_CONFIG);
	writel(0x0110, i2s->base + LS1X_IIS_VERSION);

	return 0;
}

static int ls1x_i2s_dai_remove(struct snd_soc_dai *dai)
{
	return 0;
}

#define LS1X_I2S_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_48000)

#define LS1X_I2S_FMTS (SNDRV_PCM_FMTBIT_S8 |\
	SNDRV_PCM_FMTBIT_U8 |\
	SNDRV_PCM_FMTBIT_S16_LE |\
	SNDRV_PCM_FMTBIT_S16_BE |\
	SNDRV_PCM_FMTBIT_U16_LE	|\
	SNDRV_PCM_FMTBIT_U16_BE	|\
	SNDRV_PCM_FMTBIT_S24_LE |\
	SNDRV_PCM_FMTBIT_S24_BE |\
	SNDRV_PCM_FMTBIT_U24_LE |\
	SNDRV_PCM_FMTBIT_U24_BE |\
	SNDRV_PCM_FMTBIT_S32_LE |\
	SNDRV_PCM_FMTBIT_S32_BE |\
	SNDRV_PCM_FMTBIT_U32_LE |\
	SNDRV_PCM_FMTBIT_U32_BE)

static struct snd_soc_dai_ops ls1x_i2s_dai_ops = {
	.startup	= ls1x_i2s_startup,
	.shutdown	= ls1x_i2s_shutdown,
	.trigger	= ls1x_i2s_trigger,
	.hw_params	= ls1x_i2s_hw_params,
	.set_fmt	= ls1x_i2s_set_fmt,
	.set_sysclk	= ls1x_i2s_set_sysclk,
};

struct snd_soc_dai_driver ls1x_i2s_dai = {
	.probe = ls1x_i2s_dai_probe,
	.remove = ls1x_i2s_dai_remove,
	.suspend = ls1x_i2s_dai_suspend,
	.resume = ls1x_i2s_dai_resume,
	.playback = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = LS1X_I2S_RATES,
		.formats = LS1X_I2S_FMTS,
	},
	.capture = {
		.channels_min = 1,
		.channels_max = 2,
		.rates = LS1X_I2S_RATES,
		.formats = LS1X_I2S_FMTS,
	},
	.ops = &ls1x_i2s_dai_ops,
};

static const struct snd_soc_component_driver ls1x_i2s_component = {
	.name		= "ls1x-i2s",
};

static int ls1x_i2s_dev_probe(struct platform_device *pdev)
{
	struct ls1x_i2s *i2s;
	int ret;

	i2s = kzalloc(sizeof(*i2s), GFP_KERNEL);

	if (!i2s)
		return -ENOMEM;

	i2s->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!i2s->mem) {
		ret = -ENOENT;
		goto err_free;
	}

	i2s->mem = request_mem_region(i2s->mem->start, resource_size(i2s->mem),
				pdev->name);
	if (!i2s->mem) {
		ret = -EBUSY;
		goto err_free;
	}

	i2s->base = ioremap_nocache(i2s->mem->start, resource_size(i2s->mem));
	if (!i2s->base) {
		ret = -EBUSY;
		goto err_release_mem_region;
	}

//	i2s->phys_base = i2s->mem->start;

	i2s->clk_i2s = clk_get(&pdev->dev, "apb_clk");
	if (IS_ERR(i2s->clk_i2s)) {
		ret = PTR_ERR(i2s->clk_i2s);
		goto err_iounmap;
	}

	platform_set_drvdata(pdev, i2s);

	ret = snd_soc_register_component(&pdev->dev, &ls1x_i2s_component,
					  &ls1x_i2s_dai, 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register DAI\n");
		goto err_clk_put_i2s;
	}

	return 0;

err_clk_put_i2s:
	clk_put(i2s->clk_i2s);
err_iounmap:
	iounmap(i2s->base);
err_release_mem_region:
	release_mem_region(i2s->mem->start, resource_size(i2s->mem));
err_free:
	kfree(i2s);

	return ret;
}

static int ls1x_i2s_dev_remove(struct platform_device *pdev)
{
	struct ls1x_i2s *i2s = platform_get_drvdata(pdev);

	snd_soc_unregister_component(&pdev->dev);

	clk_put(i2s->clk_i2s);

	iounmap(i2s->base);
	release_mem_region(i2s->mem->start, resource_size(i2s->mem));

	platform_set_drvdata(pdev, NULL);
	kfree(i2s);

	return 0;
}

static struct platform_driver ls1x_i2s_driver = {
	.probe = ls1x_i2s_dev_probe,
	.remove = ls1x_i2s_dev_remove,
	.driver = {
		.name = "ls1x-i2s",
		.owner = THIS_MODULE,
	},
};

static int ls1x_i2s_init(void)
{
	return platform_driver_register(&ls1x_i2s_driver);
}

static void ls1x_i2s_exit(void)
{
	platform_driver_unregister(&ls1x_i2s_driver);
}

module_init(ls1x_i2s_init);
module_exit(ls1x_i2s_exit);

/* Module information */
MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("ls1x I2S SoC Interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ls1x-i2s");
