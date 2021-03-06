/*
 * Loongson GPIO Support
 *
 * Copyright (c) 2008  Richard Liu, STMicroelectronics <richard.liu@st.com>
 * Copyright (c) 2008-2010  Arnaud Patard <apatard@mandriva.com>
 * Copyright (c) 2014  Huacai Chen <chenhc@lemote.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __LOONGSON_GPIO_H
#define __LOONGSON_GPIO_H

#include <asm-generic/gpio.h>

#define gpio_get_value __gpio_get_value
#define gpio_set_value __gpio_set_value
#define gpio_cansleep __gpio_cansleep
#define gpio_to_irq	__gpio_to_irq
#define irq_to_gpio	__irq_to_gpio

#endif	/* __LOONGSON_GPIO_H */
