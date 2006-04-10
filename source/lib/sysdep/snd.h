#ifndef SND_H__
#define SND_H__

#ifdef __cplusplus
extern "C" {
#endif

const size_t SND_CARD_LEN = 128;
extern char snd_card[SND_CARD_LEN];

const size_t SND_DRV_VER_LEN = 256;
extern char snd_drv_ver[SND_DRV_VER_LEN];

// detect sound card and set the above information.
extern void snd_detect(void);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef SND_H__
