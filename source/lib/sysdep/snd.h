/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * sound card detection.
 */

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
