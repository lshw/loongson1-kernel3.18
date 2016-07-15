#ifndef _ARCH_MCI_H
#define _ARCH_MCI_H

struct ls1x_mci_pdata {

	/* driver activation and (optional) card detect irq hookup */
	int (*init)(struct device *,
		irqreturn_t (*)(int, void *),
		void *);
	void (*exit)(struct device *, void *);

	/* sense switch on sd cards */
	int (*get_ro)(struct device *);

	/*
	 * If board does not use CD interrupts, driver can optimize polling
	 * using this function.
	 */
	int (*get_cd)(struct device *);

	/* Capabilities to pass into mmc core (e.g. MMC_CAP_NEEDS_POLL). */
	unsigned long caps;

	/* how long to debounce card detect, in msecs */
	u16 detect_delay;

	unsigned long	ocr_avail;
	void		(*set_power)(unsigned char power_mode,
				     unsigned short vdd);

	unsigned long	max_clk;
};

/**
 * ls1x_mci_set_platdata - set platform data for mmc/sdi device
 * @pdata: The platform data
 *
 * Copy the platform data supplied by @pdata so that this can be marked
 * __initdata.
 */
extern void ls1x_mci_set_platdata(struct ls1x_mci_pdata *pdata);

#endif /* _ARCH_NCI_H */
