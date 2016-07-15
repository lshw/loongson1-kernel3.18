/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LS1X_PCM_H
#define _LS1X_PCM_H

#include <linux/dma-mapping.h>

/* dma regs */
#define CONFREG_BASE	((void __iomem *)0xbfd01160)
#define DMA_ORDERED		0x0
#define DMA_SADDR		0x1
#define DMA_DADDR		0x2
#define DMA_LENGTH		0x3
#define DMA_STEP_LENGTH		0x4
#define DMA_STEP_TIMES		0x5
#define	DMA_CMD			0x6

/* bit offset in order_addr_reg */
#define DMA_STOP_OFF		0x4
#define DMA_START_OFF		0x3
#define DMA_ASKVALID_OFF	0x2
#define DMA_DEVNUM_OFF		0x0

/* define dma status */
#define DMA_START		0x0
#define DMA_STOP		0x1

typedef struct ls1x_dma_desc {
	u32 ordered;
	u32 saddr;
	u32 daddr;
	u32 length;
	u32 step_length;
	u32 step_times;
	u32 dcmd;
	u32 stats;
} ls1x_dma_desc;

#endif
