#ifndef SND_H__
#define SND_H__

const size_t SND_CARD_LEN = 128;
extern char snd_card[SND_CARD_LEN];

const size_t SND_DRV_VER_LEN = 256;
extern char snd_drv_ver[SND_DRV_VER_LEN];


extern void get_snd_info(void);

#endif	// #ifndef SND_H__
