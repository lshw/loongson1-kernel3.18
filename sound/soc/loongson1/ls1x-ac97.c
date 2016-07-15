/*
 * linux/sound/ls1x-ac97.c -- AC97 support for the Loongson1 chip.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/ac97_codec.h>
#include <sound/soc.h>

#include <sound/ls1x-lib.h>

#define LS1X_AC97_DMA_OUT	0x0fe72420
#define LS1X_AC97_DMA_IN	0x0fe74c4c

static void ls1x_ac97_warm_reset(struct snd_ac97 *ac97)
{
	ls1x_ac97_try_warm_reset(ac97);
}

static void ls1x_ac97_cold_reset(struct snd_ac97 *ac97)
{
	ls1x_ac97_try_cold_reset(ac97);
}

static struct snd_ac97_bus_ops ls1x_ac97_ops = {
	.read	= ls1x_ac97_read,
	.write	= ls1x_ac97_write,
	.warm_reset	= ls1x_ac97_warm_reset,
	.reset	= ls1x_ac97_cold_reset,
};

static struct ls1x_pcm_dma_params ls1x_ac97_pcm_stereo_out = {
	.name			= "AC97 PCM Stereo out",
	.dev_addr		= LS1X_AC97_DMA_OUT,
};

static struct ls1x_pcm_dma_params ls1x_ac97_pcm_stereo_in = {
	.name			= "AC97 PCM Stereo in",
	.dev_addr		= LS1X_AC97_DMA_IN,
};

static int ls1x_ac97_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params,
				 struct snd_soc_dai *cpu_dai)
{
	struct ls1x_pcm_dma_params *dma_data;
	uint32_t ctrl = 0;
	uint32_t conf = 0;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
	case SNDRV_PCM_FORMAT_U8:
		ctrl |= 0x00 << 28;
		conf |= 0x00 << 2;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
	case SNDRV_PCM_FORMAT_U16_BE:
		ctrl |= 0x01 << 28;
		conf |= 0x02 << 2;
		break;
/*	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_U24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
	case SNDRV_PCM_FORMAT_U24_BE:
		ctrl |= 0x02 << 28;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_U32_LE:
	case SNDRV_PCM_FORMAT_S32_BE:
	case SNDRV_PCM_FORMAT_U32_BE:
		ctrl |= 0x02 << 28;
		break;*/
	default:
		printk("\nerror SNDRV_PCM_FORMAT=%d\n", params_format(params));
		return -EINVAL;
	}

	switch (params_channels(params)) {
	case 1:
		ctrl |= 0 << 30;
		break;
	case 2:
		ctrl |= 1 << 30;
		break;
	default:
		printk("\nerror params channels = %d\n", params_channels(params));
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ls1x_ac97_channel_config_out(conf);
		ls1x_ac97_pcm_stereo_out.dev_addr &= 0x0fffffff;
		ls1x_ac97_pcm_stereo_out.dev_addr |= (ctrl | (0x1 << 31));
		dma_data = &ls1x_ac97_pcm_stereo_out;
	}
	else {
		ls1x_ac97_channel_config_in(conf);
		ls1x_ac97_pcm_stereo_in.dev_addr &= 0x0fffffff;
		ls1x_ac97_pcm_stereo_in.dev_addr |= (ctrl | (0x0 << 31));
		dma_data = &ls1x_ac97_pcm_stereo_in;
	}

	snd_soc_dai_set_dma_data(cpu_dai, substream, dma_data);

	return 0;
}


#define LS1X_AC97_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
		SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | \
		SNDRV_PCM_RATE_48000)

#define LS1X_AC97_FMTS (SNDRV_PCM_FMTBIT_S8 |\
	SNDRV_PCM_FMTBIT_U8 |\
	SNDRV_PCM_FMTBIT_S16_LE |\
	SNDRV_PCM_FMTBIT_S16_BE |\
	SNDRV_PCM_FMTBIT_U16_LE	|\
	SNDRV_PCM_FMTBIT_U16_BE)

static struct snd_soc_dai_ops ls1x_ac97_hifi_dai_ops = {
	.hw_params	= ls1x_ac97_hw_params,
};

/*
 * There is only 1 physical AC97 interface for ls1x, but it
 * has extra fifo's that can be used for aux DACs and ADCs.
 */
static struct snd_soc_dai_driver ls1x_ac97_dai_driver[] = {
{
	.name = "ls1x-ac97",
	.ac97_control = true,
	.playback = {
		.stream_name = "AC97 Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = LS1X_AC97_RATES,
		.formats = LS1X_AC97_FMTS,},
	.capture = {
		.stream_name = "AC97 Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = LS1X_AC97_RATES,
		.formats = LS1X_AC97_FMTS,},
	.ops = &ls1x_ac97_hifi_dai_ops,
},
};

static const struct snd_soc_component_driver ls1x_ac97_component = {
	.name		= "ls1x-ac97",
};

static int ls1x_ac97_dev_probe(struct platform_device *pdev)
{
	int ret;

	if (pdev->id != -1) {
		dev_err(&pdev->dev, "LS1x has only one AC97 port.\n");
		return -ENXIO;
	}

	ret = ls1x_ac97_hw_probe(pdev);
	if (ret) {
		dev_err(&pdev->dev, "ls1x AC97 hw probe error (%d)\n", ret);
		return ret;
	}

	ret = snd_soc_set_ac97_ops(&ls1x_ac97_ops);
	if (ret != 0)
		return ret;

	/* Punt most of the init to the SoC probe; we may need the machine
	 * driver to do interesting things with the clocking to get us up
	 * and running.
	 */
	return snd_soc_register_component(&pdev->dev, &ls1x_ac97_component,
					  ls1x_ac97_dai_driver, ARRAY_SIZE(ls1x_ac97_dai_driver));
}

static int ls1x_ac97_dev_remove(struct platform_device *pdev)
{
	snd_soc_unregister_component(&pdev->dev);
	snd_soc_set_ac97_ops(NULL);
	ls1x_ac97_hw_remove(pdev);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ls1x_ac97_dev_suspend(struct device *dev)
{
	return ls1x_ac97_hw_suspend();
}

static int ls1x_ac97_dev_resume(struct device *dev)
{
	return ls1x_ac97_hw_resume();
}

static SIMPLE_DEV_PM_OPS(ls1x_ac97_pm_ops,
		ls1x_ac97_dev_suspend, ls1x_ac97_dev_resume);
#endif

static struct platform_driver ls1x_ac97_driver = {
	.probe		= ls1x_ac97_dev_probe,
	.remove		= ls1x_ac97_dev_remove,
	.driver		= {
		.name	= "ls1x-ac97",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM_SLEEP
		.pm	= &ls1x_ac97_pm_ops,
#endif
	},
};

module_platform_driver(ls1x_ac97_driver);

MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("AC97 driver for the Loongson1 chip");
MODULE_LICENSE("GPL");
