#ifndef __ASM_ARM_REGS_SDI
#define __ASM_ARM_REGS_SDI "regs-sdi.h"

#define LS1X_SDICON                (0x00)
#define LS1X_SDIPRE                (0x04)
#define LS1X_SDICMDARG             (0x08)
#define LS1X_SDICMDCON             (0x0C)
#define LS1X_SDICMDSTAT            (0x10)
#define LS1X_SDIRSP0               (0x14)
#define LS1X_SDIRSP1               (0x18)
#define LS1X_SDIRSP2               (0x1C)
#define LS1X_SDIRSP3               (0x20)
#define LS1X_SDITIMER              (0x24)
#define LS1X_SDIBSIZE              (0x28)
#define LS1X_SDIDCON               (0x2C)
#define LS1X_SDIDCNT               (0x30)
#define LS1X_SDIDSTA               (0x34)
#define LS1X_SDIFSTA               (0x38)

#define LS1X_SDIIMSK               (0x3C)
#define LS1X_SDIDATA               (0x40)

#define LS1X_SDIINTEN              (0x64)

#define LS1X_SDICON_SDRESET        (1<<8)
#define LS1X_SDICON_ENCLK          (1<<0)

#define LS1X_SDICMDCON_FUNCNUM     (0x7)
#define LS1X_SDICMDCON_SDIOEN      (1<<14)
#define LS1X_SDICMDCON_CRCON       (1<<13)
#define LS1X_SDICMDCON_ABORT       (1<<12)
#define LS1X_SDICMDCON_LONGRSP     (1<<10)
#define LS1X_SDICMDCON_WAITRSP     (1<<9)
#define LS1X_SDICMDCON_CMDSTART    (1<<8)
#define LS1X_SDICMDCON_SENDERHOST  (1<<6)
#define LS1X_SDICMDCON_INDEX       (0x3f)

#define LS1X_SDICMDSTAT_CRCFAIL    (1<<12)
#define LS1X_SDICMDSTAT_CMDSENT    (1<<11)
#define LS1X_SDICMDSTAT_CMDTIMEOUT (1<<10)
#define LS1X_SDICMDSTAT_RSPFIN     (1<<9)
#define LS1X_SDICMDSTAT_XFERING    (1<<8)
#define LS1X_SDICMDSTAT_INDEX      (0xff)


#define LS1X_SDIDCON_RWRESUME      (1<<20)
#define LS1X_SDIDCON_IORESUME      (1<<19)
#define LS1X_SDIDCON_IOSUSPEND     (1<<18)
#define LS1X_SDIDCON_RWAITREQ      (1<<17)
#define LS1X_SDIDCON_WIDEBUS       (1<<16)
#define LS1X_SDIDCON_DMAEN         (1<<15)
#define LS1X_SDIDCON_STOP          (1<<14)
#define LS1X_SDIDCON_DATSTART      (1<<14)
#define LS1X_SDIDCON_BLKNUM        (0x7ff)

#define LS1X_SDIDCON_BLKNUM_MASK   (0xFFF)
#define LS1X_SDIDCNT_BLKNUM_SHIFT  (12)

#define LS1X_SDIDSTA_RWAITREQ      (1<<9)
#define LS1X_SDIDSTA_SDIOIRQDETECT (1<<8)
#define LS1X_SDIDSTA_SDIOINT       (1<<8)
#define LS1X_SDIDSTA_CRCFAIL       (1<<7)
#define LS1X_SDIDSTA_RXCRCFAIL     (1<<6)
#define LS1X_SDIDSTA_DATATIMEOUT   (1<<5)
#define LS1X_SDIDSTA_XFERFINISH    (1<<4)
#define LS1X_SDIDSTA_BUSYFINISH    (1<<3)
#define LS1X_SDIDSTA_SBITERR       (1<<2)
#define LS1X_SDIDSTA_TXDATAON      (1<<1)
#define LS1X_SDIDSTA_RXDATAON      (1<<0)

#define LS1X_SDIFSTA_TXFULL         (1<<11)
#define LS1X_SDIFSTA_TXEMPTY        (1<<10)
#define LS1X_SDIFSTA_RXFULL         (1<<8)
#define LS1X_SDIFSTA_RXEMPTY        (1<<7)

#define LS1X_SDIINTEN_BUSYFINISH    (1<<9)
#define LS1X_SDIINTEN_RESPONSECRC   (1<<8)
#define LS1X_SDIINTEN_CMDTIMEOUT    (1<<7)
#define LS1X_SDIINTEN_CMDSENT       (1<<6)
#define LS1X_SDIINTEN_SDIOIRQ       (1<<5)
#define LS1X_SDIINTEN_PROGERR       (1<<4)
#define LS1X_SDIINTEN_CRCSTATUS     (1<<3)
#define LS1X_SDIINTEN_DATACRC       (1<<2)
#define LS1X_SDIINTEN_DATATIMEOUT   (1<<1)
#define LS1X_SDIINTEN_DATAFINISH    (1<<0)

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

struct ls1x_mci_host {
	struct platform_device	*pdev;
	struct ls1x_mci_pdata	*pdata;

	struct mmc_host		*mmc;
	struct mmc_request	*mrq;
	struct mmc_data		*data;

	struct resource	*mem;
	struct clk		*clk;
	void __iomem	*base;
	int	irq;

	unsigned long		clk_rate;

	struct ls1x_dma_desc	*dma_desc;
	dma_addr_t	dma_desc_phys;
	size_t	dma_desc_size;
	unsigned int	dma_dir;
	int dma_data_err;

	spinlock_t	complete_lock;

	int bus_width;
	int power_mode;
	int clock;
};

#endif /* __ASM_ARM_REGS_SDI */
