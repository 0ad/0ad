/**
 * =========================================================================
 * File        : snd.h
 * Project     : 0 A.D.
 * Description : sound card detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_SND
#define INCLUDED_SND

const size_t SND_CARD_LEN = 128;
/**
 * description of sound card.
 **/
extern char snd_card[SND_CARD_LEN];

const size_t SND_DRV_VER_LEN = 256;
/**
 * sound driver identification and version.
 **/
extern char snd_drv_ver[SND_DRV_VER_LEN];

/**
 * detect sound card and set the above information.
 **/
extern void snd_detect(void);

#endif	// #ifndef INCLUDED_SND
