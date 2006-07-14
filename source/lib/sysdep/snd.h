/**
 * =========================================================================
 * File        : snd.h
 * Project     : 0 A.D.
 * Description : sound card detection.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef SND_H__
#define SND_H__

#ifdef __cplusplus
extern "C" {
#endif


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


#ifdef __cplusplus
}
#endif

#endif	// #ifndef SND_H__
