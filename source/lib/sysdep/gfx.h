/**
 * =========================================================================
 * File        : gfx.h
 * Project     : 0 A.D.
 * Description : graphics card detection.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef GFX_H__
#define GFX_H__

#ifdef __cplusplus
extern "C" {
#endif

const size_t GFX_CARD_LEN = 128;
extern char gfx_card[GFX_CARD_LEN];		// default: ""

const size_t GFX_DRV_VER_LEN = 256;		// increased from 64 by joe cocovich to accomodate unused drivers still in registry
extern char gfx_drv_ver[GFX_DRV_VER_LEN];	// default: ""

extern int gfx_mem;	// [MiB]; approximate

// detect graphics card and set the above information.
extern void gfx_detect(void);


// useful for choosing a video mode.
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern LibError gfx_get_video_mode(int* xres, int* yres, int* bpp, int* freq);

// useful for determining aspect ratio.
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern LibError gfx_get_monitor_size(int& width_mm, int& height_mm);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef GFX_H__
