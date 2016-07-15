/*
 * SoC audio driver for loongson1
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include "ls1x-i2s.h"

/*-------------------------  I2S PART  ---------------------------*/

static int ls1x_i2s_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	int ret;

	snd_soc_dai_set_sysclk(cpu_dai, 0, 34000000, SND_SOC_CLOCK_OUT);

//	snd_soc_dai_set_sysclk(codec_dai, 0, 22579600, SND_SOC_CLOCK_IN);

	/* codec is bitclock and lrclk master */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		goto out;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		goto out;

	ret = 0;
out:
	return ret;
}

static struct snd_soc_ops ls1x_i2s_uda1342_ops = {
	.startup	= ls1x_i2s_startup,
};

static struct snd_soc_dai_link ls1x_i2s_dai = {
	.name = "es8328",
	.stream_name = "ls1x<->i2s",
	.cpu_dai_name = "ls1x-i2s",
	.codec_dai_name = "es8328-hifi-analog",
	.platform_name = "loongson1-pcm-audio",
	.codec_name = "es8328.0-0010",
	.ops = &ls1x_i2s_uda1342_ops,
};

static struct snd_soc_card ls1x_i2s_machine = {
	.name = "LS1X_I2S",
	.dai_link = &ls1x_i2s_dai,
	.num_links = 1,
};

/*-------------------------  AC97 PART  ---------------------------*/

static struct snd_soc_dai_link ls1x_ac97_dai = {
	.name		= "AC97",
	.stream_name	= "AC97 HiFi",
	.codec_dai_name	= "ac97-hifi",
	.cpu_dai_name	= "ls1x-ac97",
	.platform_name	= "loongson1-pcm-audio",
	.codec_name	= "ac97-codec",
};

static struct snd_soc_card ls1x_ac97_machine = {
	.name		= "LS1X_AC97",
	.owner = THIS_MODULE,
	.dai_link	= &ls1x_ac97_dai,
	.num_links	= 1,
};

/*-------------------------  COMMON PART  ---------------------------*/

static struct platform_device *ls1x_snd_device;

static int __init ls1x_init(void)
{
	int ret;

	ls1x_snd_device = platform_device_alloc("soc-audio", -1);
	if (!ls1x_snd_device)
		return -ENOMEM;

#if defined(CONFIG_SND_LS1X_SOC_I2S)
	platform_set_drvdata(ls1x_snd_device, &ls1x_i2s_machine);
#elif defined(CONFIG_SND_LS1X_SOC_AC97)
	platform_set_drvdata(ls1x_snd_device, &ls1x_ac97_machine);
#endif
	ret = platform_device_add(ls1x_snd_device);

	if (ret)
		platform_device_put(ls1x_snd_device);

	return ret;
}

static void __exit ls1x_exit(void)
{
	platform_device_unregister(ls1x_snd_device);
}

module_init(ls1x_init);
module_exit(ls1x_exit);

/* Module information */
MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("ALSA SoC ls1x board Audio support");
MODULE_LICENSE("GPL");
