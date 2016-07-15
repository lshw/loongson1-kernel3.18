/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LS1X_I2S_H
#define _LS1X_I2S_H

#define LS1X_IIS_BASE(x)	((void __iomem *)(0xbfe60000 + x))

#define LS1X_IIS_VERSION	(0x00)
#define LS1X_IIS_CONFIG		(0x04)
#define LS1X_IIS_CONTROL	(0x08)
#define LS1X_IIS_RXDATA		(0x0c)
#define LS1X_IIS_TXDATA		(0x10)

#define CONTROL_MASTER	(0x8000)
#define CONTROL_MSB_LSB	(0x4000)
#define CONTROL_RX_EN	(0x2000)
#define CONTROL_TX_EN	(0x1000)
#define CONTROL_RX_DMA_EN	(0x0800)
#define CONTROL_TX_DMA_EN	(0x0080)
#define CONTROL_RX_INT_EN	(0x0002)
#define CONTROL_TX_INT_EN	(0x0001)

#endif
