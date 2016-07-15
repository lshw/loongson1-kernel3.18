/*
 *  Copyright (C) 2014, Tang Haifeng <tanghaifeng-gz@loongson.cn> or <pengren.mcu@qq.com>
 *  Loongson1 platform PWM support
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under  the terms of the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define REG_PWM_CNTR	0x00
#define REG_PWM_HRC	0x04
#define REG_PWM_LRC	0x08
#define REG_PWM_CTRL	0x0c

struct ls1x_pwm_chip {
	struct pwm_chip	chip;
	struct device	*dev;

	struct clk	*clk;
	void __iomem	*mmio_base;
};

static inline struct ls1x_pwm_chip *to_ls1x_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct ls1x_pwm_chip, chip);
}

static int ls1x_pwm_request(struct pwm_chip *chip, struct pwm_device *pwm)
{
	return 0;
}

static void ls1x_pwm_free(struct pwm_chip *chip, struct pwm_device *pwm)
{
}

static int ls1x_pwm_config(struct pwm_chip *chip, struct pwm_device *pwm,
			  int duty_ns, int period_ns)
{
	struct ls1x_pwm_chip *pc = to_ls1x_pwm_chip(chip);
	unsigned long long tmp;
	unsigned long period, duty;

	if (duty_ns < 0 || duty_ns > period_ns)
		return -EINVAL;

	tmp = (unsigned long long)clk_get_rate(pc->clk) * period_ns;
	do_div(tmp, 1000000000);
	period = tmp;

	tmp = (unsigned long long)period * duty_ns;
	do_div(tmp, period_ns);
	duty = period - tmp;

	writel(duty, pc->mmio_base + REG_PWM_HRC);
	writel(period, pc->mmio_base + REG_PWM_LRC);

	return 0;
}

static int ls1x_pwm_enable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ls1x_pwm_chip *pc = to_ls1x_pwm_chip(chip);

	writel(0x00, pc->mmio_base + REG_PWM_CNTR);
	writel(0x01, pc->mmio_base + REG_PWM_CTRL);
	return 0;
}

static void ls1x_pwm_disable(struct pwm_chip *chip, struct pwm_device *pwm)
{
	struct ls1x_pwm_chip *pc = to_ls1x_pwm_chip(chip);

	writel(0x09, pc->mmio_base + REG_PWM_CTRL);
}

static struct pwm_ops ls1x_pwm_ops = {
	.request = ls1x_pwm_request,
	.free = ls1x_pwm_free,
	.config = ls1x_pwm_config,
	.enable = ls1x_pwm_enable,
	.disable = ls1x_pwm_disable,
	.owner = THIS_MODULE,
};

static struct pwm_device *
ls1x_pwm_of_xlate(struct pwm_chip *pc, const struct of_phandle_args *args)
{
	struct pwm_device *pwm;

	pwm = pwm_request_from_chip(pc, 0, NULL);
	if (IS_ERR(pwm))
		return pwm;

	pwm_set_period(pwm, args->args[0]);

	return pwm;
}

static int ls1x_pwm_probe(struct platform_device *pdev)
{
	struct ls1x_pwm_chip *pwm;
	struct resource *r;
	int ret = 0;

	pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
	if (pwm == NULL) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}

	pwm->clk = devm_clk_get(&pdev->dev, "apb_clk");
	if (IS_ERR(pwm->clk))
		return PTR_ERR(pwm->clk);

	pwm->chip.dev = &pdev->dev;
	pwm->chip.ops = &ls1x_pwm_ops;
	pwm->chip.base = -1;
	pwm->chip.npwm = 1;

	if (IS_ENABLED(CONFIG_OF)) {
		pwm->chip.of_xlate = ls1x_pwm_of_xlate;
		pwm->chip.of_pwm_n_cells = 1;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	pwm->mmio_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(pwm->mmio_base))
		return PTR_ERR(pwm->mmio_base);

	ret = pwmchip_add(&pwm->chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "pwmchip_add() failed: %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, pwm);
	return 0;
}

static int ls1x_pwm_remove(struct platform_device *pdev)
{
	struct ls1x_pwm_chip *ls1x = platform_get_drvdata(pdev);

	if (ls1x == NULL)
		return -ENODEV;

	return pwmchip_remove(&ls1x->chip);
}

static struct platform_driver ls1x_pwm_driver = {
	.driver = {
		.name = "ls1x-pwm",
		.owner = THIS_MODULE,
	},
	.probe = ls1x_pwm_probe,
	.remove = ls1x_pwm_remove,
};
module_platform_driver(ls1x_pwm_driver);

MODULE_AUTHOR("Tang Haifeng <tanghaifeng-gz@loongson.cn>");
MODULE_DESCRIPTION("Loongson1 PWM driver");
MODULE_ALIAS("platform:ls1x-pwm");
MODULE_LICENSE("GPL");
