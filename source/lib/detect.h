// system detect
//
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef __DETECT_H__
#define __DETECT_H__

#include "lib.h"

#ifdef __cplusplus
extern "C" {
#endif



#include "sysdep/gfx.h"
#include "sysdep/os.h"
#include "sysdep/cpu.h"

extern void get_cpu_info();



const size_t SND_CARD_LEN = 16;
extern char snd_card[SND_CARD_LEN];

const size_t SND_DRV_VER_LEN = 128;
extern char snd_drv_ver[SND_DRV_VER_LEN];

extern void get_snd_info();


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __DETECT_H__
