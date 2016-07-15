/*
 * Copyright (c) 2011 Zhao Zhang <zhzhl555@gmail.com>
 * Copyright (c) 2013 Tang, Haifeng <tanghaifeng-gz@loongson.cn>
 *
 * Derived from driver/rtc/rtc-au1xxx.c
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rtc.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/io.h>
#include <asm/mach-loongson1/loongson1.h>

#define LS1X_RTC_REG_OFFSET	(LS1X_RTC_BASE + 0x20)
#define LS1X_RTC_REGS(x) \
		((void __iomem *)KSEG1ADDR(LS1X_RTC_REG_OFFSET + (x)))

/*RTC programmable counters 0 and 1*/
#define SYS_COUNTER_CNTRL		(LS1X_RTC_REGS(0x20))
#define SYS_CNTRL_ERS			(1 << 23)
#define SYS_CNTRL_RTS			(1 << 20)
#define SYS_CNTRL_RM2			(1 << 19)
#define SYS_CNTRL_RM1			(1 << 18)
#define SYS_CNTRL_RM0			(1 << 17)
#define SYS_CNTRL_RS			(1 << 16)
#define SYS_CNTRL_BP			(1 << 14)
#define SYS_CNTRL_REN			(1 << 13)
#define SYS_CNTRL_BRT			(1 << 12)
#define SYS_CNTRL_TEN			(1 << 11)
#define SYS_CNTRL_BTT			(1 << 10)
#define SYS_CNTRL_E0			(1 << 8)
#define SYS_CNTRL_ETS			(1 << 7)
#define SYS_CNTRL_32S			(1 << 5)
#define SYS_CNTRL_TTS			(1 << 4)
#define SYS_CNTRL_TM2			(1 << 3)
#define SYS_CNTRL_TM1			(1 << 2)
#define SYS_CNTRL_TM0			(1 << 1)
#define SYS_CNTRL_TS			(1 << 0)

/* Programmable Counter 0 Registers */
#define SYS_TOYTRIM		(LS1X_RTC_REGS(0))
#define SYS_TOYWRITE0		(LS1X_RTC_REGS(4))
#define SYS_TOYWRITE1		(LS1X_RTC_REGS(8))
#define SYS_TOYREAD0		(LS1X_RTC_REGS(0xC))
#define SYS_TOYREAD1		(LS1X_RTC_REGS(0x10))
#define SYS_TOYMATCH0		(LS1X_RTC_REGS(0x14))
#define SYS_TOYMATCH1		(LS1X_RTC_REGS(0x18))
#define SYS_TOYMATCH2		(LS1X_RTC_REGS(0x1C))

/* Programmable Counter 1 Registers */
#define SYS_RTCTRIM		(LS1X_RTC_REGS(0x40))
#define SYS_RTCWRITE0		(LS1X_RTC_REGS(0x44))
#define SYS_RTCREAD0		(LS1X_RTC_REGS(0x48))
#define SYS_RTCMATCH0		(LS1X_RTC_REGS(0x4C))
#define SYS_RTCMATCH1		(LS1X_RTC_REGS(0x50))
#define SYS_RTCMATCH2		(LS1X_RTC_REGS(0x54))

#define LS1X_SEC_OFFSET		(0)
#define LS1X_MIN_OFFSET		(6)
#define LS1X_HOUR_OFFSET	(12)
#define LS1X_DAY_OFFSET		(17)
#define LS1X_MONTH_OFFSET	(22)
#define LS1X_YEAR_OFFSET	(26)


#define LS1X_SEC_MASK		(0x3f)
#define LS1X_MIN_MASK		(0x3f)
#define LS1X_HOUR_MASK		(0x1f)
#define LS1X_DAY_MASK		(0x1f)
#define LS1X_MONTH_MASK		(0x0f)
#define LS1X_YEAR_MASK		(0x3f)

#define ls1x_get_sec(t)		(((t) >> LS1X_SEC_OFFSET) & LS1X_SEC_MASK)
#define ls1x_get_min(t)		(((t) >> LS1X_MIN_OFFSET) & LS1X_MIN_MASK)
#define ls1x_get_hour(t)	(((t) >> LS1X_HOUR_OFFSET) & LS1X_HOUR_MASK)
#define ls1x_get_day(t)		(((t) >> LS1X_DAY_OFFSET) & LS1X_DAY_MASK)
#define ls1x_get_month(t)	(((t) >> LS1X_MONTH_OFFSET) & LS1X_MONTH_MASK)
#define ls1x_get_year(t)	(((t) >> LS1X_YEAR_OFFSET) & LS1X_YEAR_MASK)

#define RTC_CNTR_OK (SYS_CNTRL_E0 | SYS_CNTRL_32S)

static int ls1x_rtc_read_time(struct device *dev, struct rtc_time *rtm)
{
	unsigned long v;

	v = readl(SYS_RTCREAD0);

	rtc_time_to_tm(v, rtm);

	dev_dbg(dev, "%s secs=%d, mins=%d, "
		"hours=%d, mday=%d, mon=%d, year=%d\n",
		"read", rtm->tm_sec, rtm->tm_min,
		rtm->tm_hour, rtm->tm_mday,
		rtm->tm_mon, rtm->tm_year);

	return rtc_valid_tm(rtm);
}

static int ls1x_rtc_set_time(struct device *dev, struct  rtc_time *rtm)
{
	unsigned long v, c;
	int ret = -ETIMEDOUT;

	rtc_tm_to_time(rtm, &v);
	v += 4;	/* 需要补上4秒？ */

	writel(v, SYS_RTCWRITE0);
	c = 0x10000;
	/* add timeout check counter, for more safe */
	while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RS) && --c)
		usleep_range(1000, 3000);

	if (!c) {
		dev_err(dev, "set time timeout!\n");
		goto err;
	}

	return 0;
err:
	return ret;
}

/*
 * ls1x rtc has three alarm, we only use alarm0
 * According to linux specification, only support one-shot alarm
 * no periodic alarm mode
 */
static int ls1x_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned long v;

	v = readl(SYS_RTCMATCH0);
	rtc_time_to_tm(v, &alm->time);
	alm->enabled = 1;
	
	return rtc_valid_tm(&alm->time);
}

static int ls1x_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	unsigned long v, c;
	int ret = -ETIMEDOUT;

	rtc_tm_to_time(&alm->time, &v);
	writel(v, SYS_RTCMATCH0);
	c = 0x10000;
	/* add timeout check counter, for more safe */
	while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM0) && --c)
		usleep_range(1000, 3000);

	if (!c) {
		dev_err(dev, "set time timeout!\n");
		goto err;
	}

	return 0;
err:
	return ret;
}

static int ls1x_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	return 0;
}

static irqreturn_t ls1x_rtc_tick_handler(int irq, void *rtc)
{
	unsigned long events = 0;

	events |= RTC_IRQF | RTC_UF;
	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;
}

static irqreturn_t ls1x_rtc0_handler(int irq, void *rtc)
{
	unsigned long events = 0;

	events |= RTC_IRQF | RTC_AF;
	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;
}

static irqreturn_t ls1x_rtc1_handler(int irq, void *rtc)
{
	unsigned long events = 0;

	events |= RTC_IRQF | RTC_AF;
	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;
}

static irqreturn_t ls1x_rtc2_handler(int irq, void *rtc)
{
	unsigned long events = 0;

	events |= RTC_IRQF | RTC_AF;
	rtc_update_irq(rtc, 1, events);

	return IRQ_HANDLED;
}

static struct rtc_class_ops  ls1x_rtc_ops = {
	.read_time	= ls1x_rtc_read_time,
	.set_time	= ls1x_rtc_set_time,
	.read_alarm	= ls1x_rtc_read_alarm,
	.set_alarm	= ls1x_rtc_set_alarm,
	.alarm_irq_enable	= ls1x_rtc_alarm_irq_enable,
};

static int ls1x_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtcdev;
	unsigned long v;
	int ret, i;

	writel(0x2100, SYS_COUNTER_CNTRL);
	writel(0x0, SYS_RTCMATCH0);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM0)
		usleep_range(1000, 3000);
	writel(0x0, SYS_RTCMATCH1);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM1)
		usleep_range(1000, 3000);
	writel(0x0, SYS_RTCMATCH2);
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RM2)
		usleep_range(1000, 3000);

	v = readl(SYS_COUNTER_CNTRL);
	if (!(v & RTC_CNTR_OK)) {
		dev_err(&pdev->dev, "rtc counters not working\n");
		ret = -ENODEV;
		goto err;
	}
	ret = -ETIMEDOUT;
	/* set to 1 HZ if needed */
	if (readl(SYS_RTCTRIM) != 32767) {
		v = 0x100000;
		while ((readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RTS) && --v)
			usleep_range(1000, 3000);

		if (!v) {
			dev_err(&pdev->dev, "time out\n");
			goto err;
		}
		writel(32767, SYS_RTCTRIM);
//		writel(32767, SYS_TOYTRIM);
	}
	/* this loop coundn't be endless */
	while (readl(SYS_COUNTER_CNTRL) & SYS_CNTRL_RTS)
		usleep_range(1000, 3000);

	rtcdev = rtc_device_register("ls1x-rtc", &pdev->dev,
					&ls1x_rtc_ops , THIS_MODULE);
	if (IS_ERR(rtcdev)) {
		ret = PTR_ERR(rtcdev);
		goto err;
	}

	platform_set_drvdata(pdev, rtcdev);

	for (i = 0; i < 4; i++) {
		irq_handler_t handler[] = { ls1x_rtc0_handler,
					ls1x_rtc1_handler,
					ls1x_rtc2_handler, ls1x_rtc_tick_handler };
		char *dev_name[] = {"rtc-mach0", "rtc-mach1", "rtc-mach2", "rtc-tick"};
		int irq = platform_get_irq(pdev, i);
		if (irq < 0) {
			ret = -ENXIO;
			goto err;
		} else {
			ret = request_irq(irq, handler[i], IRQF_DISABLED, dev_name[i], rtcdev);
			if (ret)
				goto err;
		}
	}

	return 0;
err:
	return ret;
}

static struct platform_driver  ls1x_rtc_driver = {
	.driver		= {
		.name	= "ls1x-rtc",
	},
	.probe		= ls1x_rtc_probe,
};

module_platform_driver(ls1x_rtc_driver);

MODULE_AUTHOR("zhao zhang <zhzhl555@gmail.com>");
MODULE_LICENSE("GPL");
