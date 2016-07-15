/*
 *  linux/drivers/mmc/ls1x_mci.h - Loongson 1C MCI driver
 *
 *  Copyright (C) 2014 Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>

#include "ls1x_mci.h"

#include <mci.h>

#define DRIVER_NAME "ls1x-mci"

#define DMA_ACCESS_ADDR	0x1fe6c040	/* DMA对SDIO操作的地址 */
#define ORDER_ADDR_IN	0x1fd01160	/* DMA配置寄存器 */
static void __iomem *order_addr_in;
#define DMA_DESC_NUM	4096	/* DMA描述符占用的字节数 7x4 */
/* DMA描述符 */
#define DMA_ORDERED		0x00
#define DMA_SADDR		0x04
#define DMA_DADDR		0x08
#define DMA_LENGTH		0x0c
#define DMA_STEP_LENGTH		0x10
#define DMA_STEP_TIMES		0x14
#define DMA_CMD		0x18

#define SDIO_USE_DMA0 0
#define SDIO_USE_DMA2 1
#define SDIO_USE_DMA3 2

/* 设置sdio控制器使用的dma通道，参考数据手册 */
#define DMA_NUM SDIO_USE_DMA3

#define MISC_CTRL		0x1fd00424
static void __iomem *misc_ctrl;

static int ls1x_mci_card_present(struct mmc_host *mmc);

static void ls1x_mci_reset(struct ls1x_mci_host *host)
{
	/* reset the hardware */
	writel(LS1X_SDICON_SDRESET, host->base + LS1X_SDICON);
	/* wait until reset it finished */
	while (readl(host->base + LS1X_SDICON) & LS1X_SDICON_SDRESET)
		;
}

static int ls1x_mci_initialize(struct ls1x_mci_host *host)
{
	ls1x_mci_reset(host);

	writel(0xffffffff, host->base + LS1X_SDIIMSK);
	writel(0xffffffff, host->base + LS1X_SDIINTEN);

	return 0;
}

static int ls1x_terminate_transfer(struct ls1x_mci_host *host)
{
	unsigned stoptries = 3;

	while (readl(host->base + LS1X_SDIDSTA) & (LS1X_SDIDSTA_TXDATAON | LS1X_SDIDSTA_RXDATAON)) {
		pr_debug("Transfer still in progress.\n");

		writel(LS1X_SDIDCON_STOP, host->base + LS1X_SDIDCON);
		ls1x_mci_initialize(host);

		if ((stoptries--) == 0) {
			pr_debug("Cannot stop the engine!\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int ls1x_prepare_engine(struct ls1x_mci_host *host)
{
	int rc;

	rc = ls1x_terminate_transfer(host);
	if (rc != 0)
		return rc;

	writel(0xffffffff, host->base + LS1X_SDICMDSTAT);
	writel(0xffffffff, host->base + LS1X_SDIDSTA);
	writel(0xffffffff, host->base + LS1X_SDIFSTA);
	writel(0x007fffff, host->base + LS1X_SDITIMER);

	return 0;
}

static void ls1x_mci_set_clk(struct ls1x_mci_host *host, struct mmc_ios *ios)
{
	unsigned long clock;
	u32 mci_psc;

	/* Set clock */
	for (mci_psc = 1; mci_psc < 0x38; mci_psc++) {
		clock = host->clk_rate / mci_psc;

		if (clock <= ios->clock)
			break;
	}

	if (mci_psc > 0x38)
		mci_psc = 0x38;

	writel(mci_psc, host->base + LS1X_SDIPRE);
}

static void ls1x_mci_send_cmd(struct ls1x_mci_host *host,
					struct mmc_command *cmd)
{
	uint32_t ccon;

	writel(cmd->arg, host->base + LS1X_SDICMDARG);

	ccon  = cmd->opcode & LS1X_SDICMDCON_INDEX;
	ccon |= LS1X_SDICMDCON_SENDERHOST | LS1X_SDICMDCON_CMDSTART;

	if (cmd->flags & MMC_RSP_PRESENT)
		ccon |= LS1X_SDICMDCON_WAITRSP;

	if (cmd->flags & MMC_RSP_136)
		ccon |= LS1X_SDICMDCON_LONGRSP;

//	if (cmd->flags & MMC_RSP_CRC)
//		ccon |= LS1X_SDICMDCON_CRCON;

	writel(ccon, host->base + LS1X_SDICMDCON);
}

static void ls1x_start_cmd(struct ls1x_mci_host *host, struct mmc_command *cmd)
{
	int rc;

	rc = ls1x_prepare_engine(host);
	if (rc != 0) {
		cmd->error = -EIO;
		printk(KERN_WARNING "%s: cmd->error = -EIO\n", __func__);
		return;
	}

	/* Send command */
	ls1x_mci_send_cmd(host, cmd);
}

static int ls1x_mci_block_transfer(struct ls1x_mci_host *host, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;
	dma_addr_t next_desc_phys;
	int dma_len, i;
//	int timeout = 5000;

	if (data->flags & MMC_DATA_READ) {
		host->dma_dir = DMA_FROM_DEVICE;
	} else {
		host->dma_dir = DMA_TO_DEVICE;
	}

	dma_len = dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
			     host->dma_dir);
	next_desc_phys = host->dma_desc_phys;

	for (i = 0; i < dma_len; i++) {
		next_desc_phys += sizeof(struct ls1x_dma_desc);
		host->dma_desc[i].ordered = ((u32)(next_desc_phys) | 0x1);
		host->dma_desc[i].saddr = sg_dma_address(&data->sg[i]);
		host->dma_desc[i].daddr = DMA_ACCESS_ADDR;
		host->dma_desc[i].length = (sg_dma_len(&data->sg[i]) + 3) / 4;
		host->dma_desc[i].step_length = 0;
		host->dma_desc[i].step_times = 1;
		if (data->flags & MMC_DATA_READ) {
			host->dma_desc[i].dcmd = 0x00000001;
		} else {
			host->dma_desc[i].dcmd = 0x00001001;
		}
	}
	host->dma_desc[dma_len - 1].ordered &= (~0x1);

	writel((host->dma_desc_phys & ~0x1F) | 0x8 | DMA_NUM, order_addr_in);	/* 启动DMA */
/*	while ((readl(order_addr_in) & 0x8) && (timeout-- > 0)) {
//		printf("%s. %x\n",__func__, readl(order_addr_in));
//		udelay(5);
	}
	if (!timeout) {
		host->dma_data_err = 1;
	}*/

	ls1x_mci_send_cmd(host, mrq->cmd);

	return 0;
}

static int ls1x_setup_data(struct ls1x_mci_host *host, struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;
	int rc;
	u32 dcon;

	host->data = data;

	rc = ls1x_prepare_engine(host);
	if (rc != 0) {
//		return rc;
	}

	dcon  = data->blocks & LS1X_SDIDCON_BLKNUM_MASK;
	dcon |= LS1X_SDIDCON_DMAEN | LS1X_SDIDCON_DATSTART;
	if (host->bus_width == MMC_BUS_WIDTH_4)
		dcon |= LS1X_SDIDCON_WIDEBUS;
	writel(dcon, host->base + LS1X_SDIDCON);

	writel(data->blksz, host->base + LS1X_SDIBSIZE);

	rc = ls1x_mci_block_transfer(host, mrq);
	if (rc != 0) {
		ls1x_terminate_transfer(host);
	}

	return rc;
}

static void ls1x_mci_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct ls1x_mci_host *host = mmc_priv(mmc);
	host->mrq = mrq;

	if (ls1x_mci_card_present(mmc) == 0) {
		printk(KERN_DEBUG "%s: no medium present\n", __func__);
		mrq->cmd->error = -ENOMEDIUM;
		mmc_request_done(mmc, mrq);
		return;
	} 

	if (mrq->data) {
		ls1x_setup_data(host, mrq);
	} else {
		ls1x_start_cmd(host, mrq->cmd);
	}
}

static void ls1x_mci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ls1x_mci_host *host = mmc_priv(mmc);
	unsigned int ret;

	if (host->power_mode != ios->power_mode) {
		switch (ios->power_mode) {
		case MMC_POWER_ON:
		case MMC_POWER_UP:
/*			ret = readl(misc_ctrl - 0x4);
			ret = ret & (~(1 << 24));
			writel(ret, misc_ctrl - 0x4);

			writel(readl(misc_ctrl) | ((DMA_NUM + 1) << 23) | (1 << 16), misc_ctrl);*/
			ls1x_mci_reset(host);
			writel(0xffffffff, host->base + LS1X_SDIIMSK);
			writel(0xffffffff, host->base + LS1X_SDIINTEN);

			if (host->pdata->set_power)
				host->pdata->set_power(ios->power_mode, ios->vdd);
			break;

		case MMC_POWER_OFF:
		default:
/*			ret = readl(misc_ctrl);
			ret = ret & (~(3 << 23)) & (~(3 << 16));
			writel(ret, misc_ctrl);

			ret = readl(misc_ctrl - 0x4);
			ret = ret | (1 << 24);
			writel(ret, misc_ctrl - 0x4);*/
			ls1x_mci_reset(host);
			writel(0xffffffff, host->base + LS1X_SDIIMSK);
			writel(0xffffffff, host->base + LS1X_SDIINTEN);

			if (host->pdata->set_power)
				host->pdata->set_power(ios->power_mode, ios->vdd);
			break;
		}

		host->power_mode = ios->power_mode;
	}

	if (host->clock != ios->clock) {
		ls1x_mci_set_clk(host, ios);
		host->clock = ios->clock;
	}
	ret = readl(host->base + LS1X_SDICON);
	/* Set CLOCK_ENABLE */
	if (ios->clock)
		ret |= LS1X_SDICON_ENCLK;
	else
		ret &= ~LS1X_SDICON_ENCLK;

	writel(ret, host->base + LS1X_SDICON);

	host->bus_width = ios->bus_width;
}

static int ls1x_mci_get_ro(struct mmc_host *mmc)
{
	struct ls1x_mci_host *host = mmc_priv(mmc);

	if (host->pdata && host->pdata->get_ro)
		return host->pdata->get_ro(mmc->parent);
	/*
	 * Board doesn't support read only detection; let the mmc core
	 * decide what to do.
	 */
	return -ENOSYS;
}

static int ls1x_mci_card_present(struct mmc_host *mmc)
{
	struct ls1x_mci_host *host = mmc_priv(mmc);

	if (host->pdata && host->pdata->get_cd)
		return host->pdata->get_cd(mmc->parent);
	return -ENOSYS;
}

/*
static void ls1x_mci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	printk("%s.\n",__func__);
}*/

static struct mmc_host_ops ls1x_mci_ops = {
	.request	= ls1x_mci_request,
	.set_ios	= ls1x_mci_set_ios,
	.get_ro		= ls1x_mci_get_ro,
	.get_cd		= ls1x_mci_card_present,
//	.enable_sdio_irq = ls1x_mci_enable_sdio_irq,
};

static void ls1x_mci_cmd_done(struct ls1x_mci_host *host, unsigned int ret)
{
	struct mmc_command *cmd = host->mrq->cmd;

	writel(0xffffffc0, host->base + LS1X_SDIIMSK);

	/* 需要标记cmd发送错误 */
	if (ret & 0x80) {
		cmd->error = -ETIMEDOUT;
	} else if ((ret & 0x40) != 0x40) {
		cmd->error = -EIO;
	}

	cmd->resp[0] = readl(host->base + LS1X_SDIRSP0);
	cmd->resp[1] = readl(host->base + LS1X_SDIRSP1);
	cmd->resp[2] = readl(host->base + LS1X_SDIRSP2);
	cmd->resp[3] = readl(host->base + LS1X_SDIRSP3);

	if (host->data && !cmd->error) {
	} else {
		mmc_request_done(host->mmc, host->mrq);
	}
}

static void ls1x_mci_data_done(struct ls1x_mci_host *host, unsigned int ret)
{
	struct mmc_data *data = host->mrq->data;
//	int timeout = 5000;

	writel(0xffffffff, host->base + LS1X_SDIIMSK);

	/* 需要标记data收发错误 */
	if (ret & 0x0c) {
		data->error = -ETIMEDOUT;
		printk(KERN_WARNING "data sdiimsk err:%x\n", ret);
	} else if ((ret & 0x01) != 0x01) {
		data->error = -EIO;
		printk(KERN_WARNING "data sdiimsk err:%x\n", ret);
	}

/*	writel((host->dma_desc_phys & (~0x1F)) | 0x4 | DMA_NUM, order_addr_in);
	while ((readl(order_addr_in) & 0x4) && (timeout-- > 0)) {
	}
	if (!timeout) {
		host->dma_data_err = 1;
	}

	if (host->dma_data_err) {
		data->error = -EIO;
		printk(KERN_WARNING "sdio dma data err\n");
	}*/

	dma_unmap_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
		     host->dma_dir);

	if (!data->error)
		data->bytes_xfered = data->blocks * data->blksz;
	else
		data->bytes_xfered = 0;

	host->data = NULL;
//	host->dma_data_err = 0;
	if (host->mrq->stop) {
		ls1x_mci_send_cmd(host, host->mrq->stop);
	} else {
		mmc_request_done(host->mmc, host->mrq);
	}
}

static irqreturn_t ls1x_mci_irq(int irq, void *dev_id)
{
	struct ls1x_mci_host *host = dev_id;
	int ret;
	unsigned long iflags;

	spin_lock_irqsave(&host->complete_lock, iflags);

	ret = readl(host->base + LS1X_SDIIMSK);
	if (ret & 0x1c0) {
		ls1x_mci_cmd_done(host, ret);
	} else if (ret & 0x01f) {
		ls1x_mci_data_done(host, ret);
	} else {
		writel(0xffffffff, host->base + LS1X_SDIIMSK);
		printk(KERN_WARNING "%s. %x\n",__func__, ret);
	}

	spin_unlock_irqrestore(&host->complete_lock, iflags);
	return IRQ_HANDLED;
}

static irqreturn_t ls1x_mci_irq_cd(int irq, void *mmc)
{
	struct ls1x_mci_host *host = mmc_priv(mmc);
	u16 delay_msec = max(host->pdata->detect_delay, (u16)100);

	mmc_detect_change(mmc, msecs_to_jiffies(delay_msec));
	return IRQ_HANDLED;
}

static int ls1x_mmc_dma_init(struct ls1x_mci_host *host)
{
	struct platform_device *pdev = host->pdev;
//	int ret;

	host->dma_desc_size = PAGE_ALIGN(DMA_DESC_NUM);
	host->dma_desc = dma_alloc_coherent(&pdev->dev, host->dma_desc_size,
						&host->dma_desc_phys, GFP_KERNEL);
	if (!host->dma_desc) {
		dev_err(&pdev->dev,"fialed to allocate memory\n");
		return -ENOMEM;
	}

/*	ret = request_irq(LS1X_DMA2_IRQ, ls1x_mci_dma_irq, IRQF_DISABLED, "sdio-dma", host);
	if (ret < 0)
		return ret;*/

	order_addr_in = ioremap(ORDER_ADDR_IN, 0x4);

	writel(0, host->dma_desc + DMA_ORDERED);
//	writel(p, host->dma_desc + DMA_SADDR);
	writel(DMA_ACCESS_ADDR, host->dma_desc + DMA_DADDR);
//	writel((data->blocksize * data->blocks + 3) / 4, host->dma_desc + DMA_LENGTH);
	writel(0, host->dma_desc + DMA_STEP_LENGTH);
	writel(1, host->dma_desc + DMA_STEP_TIMES);

	return 0;
}

#ifdef CONFIG_CPU_FREQ

static int ls1x_mci_cpufreq_transition(struct notifier_block *nb,
				     unsigned long val, void *data)
{
	struct ls1x_mci_host *host;
	struct mmc_host *mmc;
	unsigned long newclk;
	unsigned long flags;

	host = container_of(nb, struct ls1x_mci_host, freq_transition);
	newclk = clk_get_rate(host->clk);
	mmc = host->mmc;

	if ((val == CPUFREQ_PRECHANGE && newclk > host->clk_rate) ||
	    (val == CPUFREQ_POSTCHANGE && newclk < host->clk_rate)) {
		spin_lock_irqsave(&mmc->lock, flags);

		host->clk_rate = newclk;

		if (mmc->ios.power_mode != MMC_POWER_OFF &&
		    mmc->ios.clock != 0)
			ls1x_mci_set_clk(host, &mmc->ios);

		spin_unlock_irqrestore(&mmc->lock, flags);
	}

	return 0;
}

static inline int ls1x_mci_cpufreq_register(struct ls1x_mci_host *host)
{
	host->freq_transition.notifier_call = ls1x_mci_cpufreq_transition;

	return cpufreq_register_notifier(&host->freq_transition,
					 CPUFREQ_TRANSITION_NOTIFIER);
}

static inline void ls1x_mci_cpufreq_deregister(struct ls1x_mci_host *host)
{
	cpufreq_unregister_notifier(&host->freq_transition,
				    CPUFREQ_TRANSITION_NOTIFIER);
}

#else
static inline int ls1x_mci_cpufreq_register(struct ls1x_mci_host *host)
{
	return 0;
}

static inline void ls1x_mci_cpufreq_deregister(struct ls1x_mci_host *host)
{
}
#endif

static int ls1x_mci_probe(struct platform_device *pdev)
{
	struct ls1x_mci_host *host;
	struct mmc_host	*mmc;
	int ret;

	/* 设置复用 */
	misc_ctrl = ioremap(MISC_CTRL, 0x4);
	writel(readl(misc_ctrl) | ((DMA_NUM + 1) << 23) | (1 << 16), misc_ctrl);

	mmc = mmc_alloc_host(sizeof(struct ls1x_mci_host), &pdev->dev);
	if (!mmc) {
		ret = -ENOMEM;
		goto probe_out;
	}

	host = mmc_priv(mmc);
	host->mmc 	= mmc;
	host->pdev	= pdev;
	host->pdata = pdev->dev.platform_data;

	spin_lock_init(&host->complete_lock);

	host->mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!host->mem) {
		dev_err(&pdev->dev,
			"failed to get io memory region resource.\n");

		ret = -ENOENT;
		goto probe_free_host;
	}

	host->mem = request_mem_region(host->mem->start,
				       resource_size(host->mem), pdev->name);

	if (!host->mem) {
		dev_err(&pdev->dev, "failed to request io memory region.\n");
		ret = -ENOENT;
		goto probe_free_host;
	}

	host->base = ioremap(host->mem->start, resource_size(host->mem));
	if (!host->base) {
		dev_err(&pdev->dev, "failed to ioremap() io memory region.\n");
		ret = -EINVAL;
		goto probe_free_mem_region;
	}

	host->irq = platform_get_irq(pdev, 0);
	if (host->irq == 0) {
		dev_err(&pdev->dev, "failed to get interrupt resource.\n");
		ret = -EINVAL;
		goto probe_iounmap;
	}

	if (request_irq(host->irq, ls1x_mci_irq, 0, DRIVER_NAME, host)) {
		dev_err(&pdev->dev, "failed to request mci interrupt.\n");
		ret = -ENOENT;
		goto probe_iounmap;
	}

	disable_irq(host->irq);

	/* register card detect irq */
	if (host->pdata && host->pdata->init) {
		ret = host->pdata->init(&pdev->dev, ls1x_mci_irq_cd, mmc);
		if (ret != 0)
			goto probe_free_irq;
	}

	/* depending on the dma state, get a dma channel to use. */
	if (ls1x_mmc_dma_init(host)) {
		dev_err(&pdev->dev, "failed to init dma. \n");
		goto probe_free_irq;
	}

	host->clk = clk_get(&pdev->dev, "ls1x_sdio");
	if (IS_ERR(host->clk)) {
		dev_err(&pdev->dev, "failed to find clock source.\n");
		ret = PTR_ERR(host->clk);
		host->clk = NULL;
		goto probe_free_dma;
	}

/*	ret = clk_enable(host->clk);
	if (ret) {
		dev_err(&pdev->dev, "failed to enable clock source.\n");
		goto clk_free;
	}*/

	host->clk_rate = clk_get_rate(host->clk);

	/* pass platform capabilities, if any */
	mmc->caps = 0;
	if (host->pdata)
		mmc->caps |= host->pdata->caps;

	mmc->ops 	= &ls1x_mci_ops;
	mmc->ocr_avail	= MMC_VDD_32_33 | MMC_VDD_33_34;
//	mmc->caps |= MMC_CAP_4_BIT_DATA | MMC_CAP_SDIO_IRQ;
	mmc->caps |= MMC_CAP_4_BIT_DATA;
	mmc->f_min = host->clk_rate / 0x38;	/* 设置成256有问题？ */
	if (host->pdata->max_clk)
		mmc->f_max = host->pdata->max_clk;
	else 
		mmc->f_max = host->clk_rate / 1;

	if (host->pdata->ocr_avail)
		mmc->ocr_avail = host->pdata->ocr_avail;

	mmc->max_blk_count	= 4096;
	mmc->max_blk_size	= 4096;
	mmc->max_req_size	= 4096 * 512;
	mmc->max_seg_size	= mmc->max_req_size;
	mmc->max_segs		= 128;

	dev_info(&pdev->dev,
	    "probe: mode:ls1x mapped mci_base:%p irq:%u dma:%u.\n",
	    host->base, host->irq, DMA_NUM);

	ret = ls1x_mci_cpufreq_register(host);
	if (ret) {
		dev_err(&pdev->dev, "failed to register cpufreq\n");
		goto free_dmabuf;
	}

	ls1x_mci_reset(host);
	writel(0xffffffff, host->base + LS1X_SDIIMSK);
	writel(0xffffffff, host->base + LS1X_SDIINTEN);
	enable_irq(host->irq);

	ret = mmc_add_host(mmc);
	if (ret) {
		dev_err(&pdev->dev, "failed to add mmc host.\n");
		goto free_cpufreq;
	}

	platform_set_drvdata(pdev, mmc);
	dev_info(&pdev->dev, "%s - using %s, %s SDIO IRQ\n", mmc_hostname(mmc),
		 "dma", mmc->caps & MMC_CAP_SDIO_IRQ ? "hw" : "sw");

	return 0;

 free_cpufreq:
	ls1x_mci_cpufreq_deregister(host);

 free_dmabuf:
//	clk_disable(host->clk);

 clk_free:
//	clk_put(host->clk);

 probe_free_dma:
 	dma_free_coherent(&pdev->dev, host->dma_desc_size, host->dma_desc, host->dma_desc_phys);

 probe_free_irq:
	free_irq(host->irq, host);

 probe_iounmap:
	iounmap(host->base);

 probe_free_mem_region:
	release_mem_region(host->mem->start, resource_size(host->mem));

 probe_free_host:
	mmc_free_host(mmc);

 probe_out:
	return ret;
}

static void ls1x_mci_shutdown(struct platform_device *pdev)
{
	struct mmc_host	*mmc = platform_get_drvdata(pdev);
	struct ls1x_mci_host *host = mmc_priv(mmc);

	/* prevent new mmc_detect_change() calls */
	if (host->pdata && host->pdata->exit)
		host->pdata->exit(&pdev->dev, mmc);

//	ls1x_mci_debugfs_remove(host);
	ls1x_mci_cpufreq_deregister(host);
	mmc_remove_host(mmc);
//	clk_disable(host->clk);
}

static int ls1x_mci_remove(struct platform_device *pdev)
{
	struct mmc_host		*mmc  = platform_get_drvdata(pdev);
	struct ls1x_mci_host	*host = mmc_priv(mmc);

	ls1x_mci_shutdown(pdev);

//	clk_put(host->clk);

	dma_free_coherent(&pdev->dev, host->dma_desc_size, host->dma_desc, host->dma_desc_phys);

	free_irq(host->irq, host);

	iounmap(host->base);
	release_mem_region(host->mem->start, resource_size(host->mem));

	mmc_free_host(mmc);

	return 0;
}

static struct platform_device_id ls1x_mci_driver_ids[] = {
	{
		.name	= "ls1x-sdi",
		.driver_data	= 0,
	}, 
	{ }
};

MODULE_DEVICE_TABLE(platform, ls1x_mci_driver_ids);

static struct platform_driver ls1x_mci_driver = {
	.driver	= {
		.name	= "ls1x-sdi",
		.owner	= THIS_MODULE,
	},
	.id_table	= ls1x_mci_driver_ids,
	.probe		= ls1x_mci_probe,
	.remove		= ls1x_mci_remove,
	.shutdown	= ls1x_mci_shutdown,
};

module_platform_driver(ls1x_mci_driver);

MODULE_DESCRIPTION("Loongson 1C MMC/SD Card Interface driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
