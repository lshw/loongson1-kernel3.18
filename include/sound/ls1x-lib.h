#ifndef LS1X_LIB_H
#define LS1X_LIB_H

#include <linux/platform_device.h>
#include <sound/ac97_codec.h>

/* PCM */
struct ls1x_pcm_dma_params {
	char *name;			/* stream identifier */
//	u32 dcmd;			/* DMA descriptor dcmd field */
//	volatile u32 *drcmr;		/* the DMA request channel to use */
	u32 dev_addr;			/* device physical address for DMA */
};

/* AC97 */
extern unsigned short ls1x_ac97_read(struct snd_ac97 *ac97, unsigned short reg);
extern void ls1x_ac97_write(struct snd_ac97 *ac97, unsigned short reg, unsigned short val);

extern bool ls1x_ac97_try_warm_reset(struct snd_ac97 *ac97);
extern bool ls1x_ac97_try_cold_reset(struct snd_ac97 *ac97);

extern int ls1x_ac97_hw_suspend(void);
extern int ls1x_ac97_hw_resume(void);

extern int ls1x_ac97_hw_probe(struct platform_device *dev);
extern void ls1x_ac97_hw_remove(struct platform_device *dev);

extern void ls1x_ac97_channel_config_out(uint32_t conf);
extern void ls1x_ac97_channel_config_in(uint32_t conf);

#endif
