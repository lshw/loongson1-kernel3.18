/*
 * ALSA PCM interface for the Loongson1 chip
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <sound/ls1x-lib.h>

#include "ls1x-pcm.h"

static struct snd_pcm_substream *substream_p;

struct ls1x_runtime_data {
	struct ls1x_pcm_dma_params *params;

	ls1x_dma_desc *dma_desc_array;
	dma_addr_t dma_desc_array_phys;

	ls1x_dma_desc *dma_desc_pos;
	dma_addr_t dma_desc_pos_phys;
};

static const struct snd_pcm_hardware ls1x_pcm_hardware = {
	.info = SNDRV_PCM_INFO_MMAP |
			SNDRV_PCM_INFO_MMAP_VALID |
			SNDRV_PCM_INFO_INTERLEAVED |
			SNDRV_PCM_INFO_RESUME |
			SNDRV_PCM_INFO_PAUSE,
	.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_U8 | 
			SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE |
			SNDRV_PCM_FMTBIT_U16_LE	| SNDRV_PCM_FMTBIT_U16_BE |
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE |
			SNDRV_PCM_FMTBIT_U24_LE | SNDRV_PCM_FMTBIT_U24_BE |
			SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_S32_BE |
			SNDRV_PCM_FMTBIT_U32_LE | SNDRV_PCM_FMTBIT_U32_BE,
	.period_bytes_min	= 4,
	.period_bytes_max	= 2048,
	.periods_min		= 1,
	.periods_max		= PAGE_SIZE/sizeof(ls1x_dma_desc),
	.buffer_bytes_max	= 128 * 1024,
	.fifo_size			= 128,
};

static irqreturn_t ls1x_dma_irq(int irq, void *dev_id)
{
	snd_pcm_period_elapsed(substream_p);

	return IRQ_HANDLED;
}

static int ls1x_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ls1x_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct ls1x_pcm_dma_params *dma;
	size_t totsize = params_buffer_bytes(params);
	size_t period = params_period_bytes(params);
	ls1x_dma_desc *dma_desc;
	dma_addr_t dma_buff_phys, next_desc_phys;

	dma = snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!dma)
		return 0;

	/* this may get called several times by oss emulation
	 * with different params */
	if (prtd->params != dma || prtd->params == NULL) {
		prtd->params = dma;
		substream_p = substream;
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	runtime->dma_bytes = totsize;

	dma_desc = prtd->dma_desc_array;
	next_desc_phys = prtd->dma_desc_array_phys;
	dma_buff_phys = runtime->dma_addr;
	do {
		next_desc_phys += sizeof(ls1x_dma_desc);
		dma_desc->ordered = ((u32)(next_desc_phys) | 0x1);
		dma_desc->saddr = dma_buff_phys;
		dma_desc->daddr = prtd->params->dev_addr;
		dma_desc->dcmd = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ?
				0x00001001 : 0x00000001;
		dma_desc->step_length = 0;
		if (period > totsize)
			period = totsize;
//		dma_desc->length = (period + 3) / 4;
//		dma_desc->step_times = 1;
		dma_desc->length = 8;
		dma_desc->step_times = period >> 5;

		dma_desc++;
		dma_buff_phys += period;
	} while (totsize -= period);
	dma_desc[-1].ordered = ((u32)(prtd->dma_desc_array_phys) | 0x1);

	return 0;
}

static int ls1x_pcm_hw_free(struct snd_pcm_substream *substream)
{
//	struct ls1x_runtime_data *prtd = substream->runtime->private_data;

	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}

static int ls1x_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct ls1x_runtime_data *prtd = substream->runtime->private_data;

	if (!prtd->params)
		return -EBUSY;

	return 0;
}

static int ls1x_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct ls1x_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;
	u32 val;
	
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		val = prtd->dma_desc_array_phys;
		val |= (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? 0x9 : 0xa;
		writel(val, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x8);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		//写回控制器寄存器的值,暂停播放
		val = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? 0x5 : 0x6;
		writel(prtd->dma_desc_pos_phys | val, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x4);
		val = (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? 0x11 : 0x12;
		writel(prtd->dma_desc_pos_phys | val, CONFREG_BASE);
		break;
	case SNDRV_PCM_TRIGGER_RESUME:
		val = readl(CONFREG_BASE);
		val |=  (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? 0x9 : 0xa;
		writel(val, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x8);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		val = readl(CONFREG_BASE);
		val |= (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) ? 0x9 : 0xa;
		writel(val, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x8);
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static snd_pcm_uframes_t ls1x_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ls1x_runtime_data *prtd = runtime->private_data;
	snd_pcm_uframes_t x;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		writel((prtd->dma_desc_pos_phys & (~0x1F)) | 0x5, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x4);
	} else {
		writel((prtd->dma_desc_pos_phys & (~0x1F)) | 0x6, CONFREG_BASE);
		while (readl(CONFREG_BASE) & 0x4);
	}
	x = bytes_to_frames(runtime, prtd->dma_desc_pos->saddr - runtime->dma_addr);
	if (x >= runtime->buffer_size)
		x = 0;

	return x;
}

static int ls1x_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ls1x_runtime_data *prtd;
	int ret;

	runtime->hw = ls1x_pcm_hardware;

	ret = -ENOMEM;
	prtd = kzalloc(sizeof(*prtd), GFP_KERNEL);
	if (!prtd)
		goto out;

	snd_soc_set_runtime_hwparams(substream, &ls1x_pcm_hardware);

	/* DMA描述符 */
	prtd->dma_desc_array =
		dma_alloc_coherent(substream->pcm->card->dev, PAGE_SIZE,
				       &prtd->dma_desc_array_phys, GFP_KERNEL);
	if (!prtd->dma_desc_array)
		goto err1;

	/* 用于记录当前dma描述符位置 */
	prtd->dma_desc_pos =
		dma_alloc_coherent(substream->pcm->card->dev, 32,
					&prtd->dma_desc_pos_phys, GFP_KERNEL);
	if (!prtd->dma_desc_pos)
		goto err1;

	runtime->private_data = prtd;

	return 0;

err1:
	kfree(prtd);
out:
	return ret;
}

static int ls1x_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct ls1x_runtime_data *prtd = runtime->private_data;

	dma_free_coherent(substream->pcm->card->dev, 32,	
			      prtd->dma_desc_pos, prtd->dma_desc_pos_phys);
	dma_free_coherent(substream->pcm->card->dev, PAGE_SIZE,	
			      prtd->dma_desc_array, prtd->dma_desc_array_phys);
	kfree(prtd);
	return 0;
}

static int ls1x_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	return remap_pfn_range(vma, vma->vm_start,
			substream->dma_buffer.addr >> PAGE_SHIFT,
			vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static struct snd_pcm_ops ls1x_pcm_ops = {
	.open		= ls1x_pcm_open,
	.close		= ls1x_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= ls1x_pcm_hw_params,
	.hw_free	= ls1x_pcm_hw_free,
	.prepare	= ls1x_pcm_prepare,
	.trigger	= ls1x_pcm_trigger,
	.pointer	= ls1x_pcm_pointer,
	.mmap		= ls1x_pcm_mmap,
};

static int ls1x_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = ls1x_pcm_hardware.buffer_bytes_max;

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	buf->area = dma_alloc_noncoherent(pcm->card->dev, size,
					  &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;

	buf->bytes = size;

	return 0;
}

static void ls1x_soc_pcm_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_noncoherent(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 ls1x_pcm_dmamask = DMA_BIT_MASK(32);

static int ls1x_soc_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &ls1x_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = ls1x_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = ls1x_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

static struct snd_soc_platform_driver ls1x_soc_platform = {
	.ops 	= &ls1x_pcm_ops,
	.pcm_new	= ls1x_soc_pcm_new,
	.pcm_free	= ls1x_soc_pcm_free,
};

static int ls1x_soc_platform_probe(struct platform_device *pdev)
{
	int ret;

	ret = request_irq(LS1X_DMA1_IRQ, ls1x_dma_irq, IRQF_DISABLED, "pcm-dma-write", NULL);
	if (ret < 0)
		return ret;

	ret = request_irq(LS1X_DMA2_IRQ, ls1x_dma_irq, IRQF_DISABLED, "pcm-dma-read", NULL);
	if (ret < 0)
		return ret;

	return snd_soc_register_platform(&pdev->dev, &ls1x_soc_platform);
}

static int ls1x_soc_platform_remove(struct platform_device *pdev)
{
	free_irq(LS1X_DMA1_IRQ, NULL);
	free_irq(LS1X_DMA2_IRQ, NULL);

	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

static struct platform_driver loongson1_pcm_driver = {
	.driver = {
			.name = "loongson1-pcm-audio",
			.owner = THIS_MODULE,
	},

	.probe = ls1x_soc_platform_probe,
	.remove = ls1x_soc_platform_remove,
};

static int __init snd_loongson1_pcm_init(void)
{
	return platform_driver_register(&loongson1_pcm_driver);
}
module_init(snd_loongson1_pcm_init);

static void __exit snd_loongson1_pcm_exit(void)
{
	platform_driver_unregister(&loongson1_pcm_driver);
}
module_exit(snd_loongson1_pcm_exit);

MODULE_AUTHOR("loongson.cn");
MODULE_DESCRIPTION("Loongson1 PCM DMA module");
MODULE_LICENSE("GPL");
