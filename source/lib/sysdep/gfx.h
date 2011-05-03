/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * graphics card detection.
 */

#ifndef INCLUDED_GFX
#define INCLUDED_GFX

const size_t GFX_CARD_LEN = 128;
/**
 * description of graphics card.
 * initial value is "".
 **/
extern wchar_t gfx_card[GFX_CARD_LEN];

// note: increased from 64 by Joe Cocovich; this large size is necessary
// because there must be enough space to list the versions of all drivers
// mentioned in the registry (including unused remnants).
const size_t GFX_DRV_VER_LEN = 256;
/**
 * (OpenGL) graphics driver identification and version.
 * initial value is "".
 **/
extern wchar_t gfx_drv_ver[GFX_DRV_VER_LEN];

/**
 * approximate amount of graphics memory [MiB]
 **/
extern int gfx_mem;

/**
 * detect graphics card and set the above information.
 **/
extern void gfx_detect();


/**
 * get current video mode.
 *
 * this is useful when choosing a new video mode.
 *
 * @param xres, yres (optional out) resolution [pixels]
 * @param bpp (optional out) bits per pixel
 * @param freq (optional out) vertical refresh rate [Hz]
 * @return Status; INFO::OK unless: some information was requested
 * (i.e. pointer is non-NULL) but cannot be returned.
 * on failure, the outputs are all left unchanged (they are
 * assumed initialized to defaults)
 **/
extern Status gfx_get_video_mode(int* xres, int* yres, int* bpp, int* freq);

/**
 * get monitor dimensions.
 *
 * this is useful for determining aspect ratio.
 *
 * @param width_mm (out) screen width [mm]
 * @param height_mm (out) screen height [mm]
 * @return Status. on failure, the outputs are all left unchanged
 * on failure, the outputs are all left unchanged (they are
 * assumed initialized to defaults)
 **/
extern Status gfx_get_monitor_size(int& width_mm, int& height_mm);

#endif	// #ifndef INCLUDED_GFX
