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

#include "ps/CStr.h"


//-----------------------------------------------------------------------------
// prevent various OpenGL features from being used. this allows working
// around issues like buggy drivers.

// when loading S3TC-compressed texture files, do not pass them directly to 
// OpenGL; instead, decompress them via software to regular textures.
// (necessary on JW's S3 laptop graphics card -- oh, the irony)
extern bool g_NoGLS3TC;

// do not ask OpenGL to create mipmaps; instead, generate them in software
// and upload them all manually. (potentially helpful for PT's system, where
// Mesa falsely reports full S3TC support but isn't able to generate mipmaps
// for them)
extern bool g_NoGLAutoMipmap;

// don't use VBOs. (RC: that was necessary on laptop Radeon cards)
extern bool g_NoGLVBO;

// disable FBO extension in case the driver is flaky
extern bool g_NoGLFramebufferObject;

//-----------------------------------------------------------------------------

// flag to switch on shadows
extern bool g_Shadows;
// flag to switch on reflective/refractive water
extern bool g_FancyWater;
// flag to switch on fixed frame timing (RC: I'm using this for profiling purposes)
extern bool g_FixedFrameTiming;

extern float g_LodBias;
extern float g_Gamma;
extern bool g_EntGraph;
// name of configured render path (depending on OpenGL extensions, this may not be
// the render path that is actually in use right now)
extern CStr g_RenderPath;

extern int g_xres, g_yres;
extern int g_bpp;
extern int g_freq;
extern bool g_VSync;

extern bool g_Quickstart;

// If non-empty, specified map will be automatically loaded
extern CStr g_AutostartMap;

extern CStrW g_CursorName;

class CmdLineArgs;
extern void CONFIG_Init(const CmdLineArgs& args);

extern bool g_ShowPathfindingOverlay;

extern bool g_TriPathfind;
