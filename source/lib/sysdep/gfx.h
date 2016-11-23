/* Copyright (c) 2013 Wildfire Games
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

namespace gfx {

/**
 * @return description of graphics card,
*  or L"" if unknown.
 **/
LIB_API std::wstring CardName();

/**
 * @return string describing the graphics driver and its version,
 * or L"" if unknown.
 **/
LIB_API std::wstring DriverInfo();

/**
 * not implemented
 **/
LIB_API size_t MemorySizeMiB();

/**
 * (useful for choosing a new video mode)
 *
 * @param xres, yres (optional out) resolution [pixels]
 * @param bpp (optional out) bits per pixel
 * @param freq (optional out) vertical refresh rate [Hz]
 * @return Status (if negative, outputs were left unchanged)
 **/
LIB_API Status GetVideoMode(int* xres, int* yres, int* bpp, int* freq);

/**
 * (useful for determining aspect ratio)
 *
 * @param width_mm (out) screen width [mm]
 * @param height_mm (out) screen height [mm]
 * @return Status (if if negative, outputs were left unchanged)
 **/
LIB_API Status GetMonitorSize(int& width_mm, int& height_mm);

}	// namespace gfx

#endif	// #ifndef INCLUDED_GFX
