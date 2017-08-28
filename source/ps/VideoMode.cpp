/* Copyright (C) 2017 Wildfire Games.
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

#include "precompiled.h"

#include "VideoMode.h"

#include "graphics/Camera.h"
#include "graphics/GameView.h"
#include "gui/GUIManager.h"
#include "lib/config2.h"
#include "lib/ogl.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/sysdep/gfx.h"
#include "lib/tex/tex.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "renderer/Renderer.h"

#if OS_MACOSX
# include "lib/sysdep/os/osx/osx_sys_version.h"
#endif


static int DEFAULT_WINDOW_W = 1024;
static int DEFAULT_WINDOW_H = 768;

static int DEFAULT_FULLSCREEN_W = 1024;
static int DEFAULT_FULLSCREEN_H = 768;

CVideoMode g_VideoMode;

CVideoMode::CVideoMode() :
	m_IsFullscreen(false), m_IsInitialised(false), m_Window(NULL),
	m_PreferredW(0), m_PreferredH(0), m_PreferredBPP(0), m_PreferredFreq(0),
	m_ConfigW(0), m_ConfigH(0), m_ConfigBPP(0), m_ConfigFullscreen(false), m_ConfigForceS3TCEnable(true),
	m_WindowedW(DEFAULT_WINDOW_W), m_WindowedH(DEFAULT_WINDOW_H), m_WindowedX(0), m_WindowedY(0)
{
	// (m_ConfigFullscreen defaults to false, so users don't get stuck if
	// e.g. half the filesystem is missing and the config files aren't loaded)
}

void CVideoMode::ReadConfig()
{
	bool windowed = !m_ConfigFullscreen;
	CFG_GET_VAL("windowed", windowed);
	m_ConfigFullscreen = !windowed;

	CFG_GET_VAL("xres", m_ConfigW);
	CFG_GET_VAL("yres", m_ConfigH);
	CFG_GET_VAL("bpp", m_ConfigBPP);
	CFG_GET_VAL("display", m_ConfigDisplay);
	CFG_GET_VAL("force_s3tc_enable", m_ConfigForceS3TCEnable);
}

bool CVideoMode::SetVideoMode(int w, int h, int bpp, bool fullscreen)
{
	Uint32 flags = 0;
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	if (!m_Window)
	{
		// Note: these flags only take affect in SDL_CreateWindow
		flags |= SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
		m_WindowedX = m_WindowedY = SDL_WINDOWPOS_CENTERED_DISPLAY(m_ConfigDisplay);

		m_Window = SDL_CreateWindow("0 A.D.", m_WindowedX, m_WindowedY, w, h, flags);
		if (!m_Window)
		{
			// If fullscreen fails, try windowed mode
			if (fullscreen)
			{
				LOGWARNING("Failed to set the video mode to fullscreen for the chosen resolution "
					"%dx%d:%d (\"%hs\"), falling back to windowed mode",
					w, h, bpp, SDL_GetError());
				// Using default size for the window for now, as the attempted setting
				// could be as large, or larger than the screen size.
				return SetVideoMode(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, bpp, false);
			}
			else
			{
				LOGERROR("SetVideoMode failed in SDL_CreateWindow: %dx%d:%d %d (\"%s\")",
					w, h, bpp, fullscreen ? 1 : 0, SDL_GetError());
				return false;
			}
		}

		if (SDL_SetWindowDisplayMode(m_Window, NULL) < 0)
		{
			LOGERROR("SetVideoMode failed in SDL_SetWindowDisplayMode: %dx%d:%d %d (\"%s\")",
				w, h, bpp, fullscreen ? 1 : 0, SDL_GetError());
			return false;
		}

		SDL_GLContext context = SDL_GL_CreateContext(m_Window);
		if (!context)
		{
			LOGERROR("SetVideoMode failed in SDL_GL_CreateContext: %dx%d:%d %d (\"%s\")",
				w, h, bpp, fullscreen ? 1 : 0, SDL_GetError());
			return false;
		}
	}
	else
	{
		if (m_IsFullscreen != fullscreen)
		{
			if (!fullscreen)
			{
				// For some reason, when switching from fullscreen to windowed mode,
				// we have to set the window size and position before and after switching
				SDL_SetWindowSize(m_Window, w, h);
				SDL_SetWindowPosition(m_Window, m_WindowedX, m_WindowedY);
			}

			if (SDL_SetWindowFullscreen(m_Window, flags) < 0)
			{
				LOGERROR("SetVideoMode failed in SDL_SetWindowFullscreen: %dx%d:%d %d (\"%s\")",
					w, h, bpp, fullscreen ? 1 : 0, SDL_GetError());
				return false;
			}
		}

		if (!fullscreen)
		{
			SDL_SetWindowSize(m_Window, w, h);
			SDL_SetWindowPosition(m_Window, m_WindowedX, m_WindowedY);
		}
	}

	// Grab the current video settings
	SDL_GetWindowSize(m_Window, &m_CurrentW, &m_CurrentH);
	m_CurrentBPP = bpp;

	if (fullscreen)
		SDL_SetWindowGrab(m_Window, SDL_TRUE);
	else
		SDL_SetWindowGrab(m_Window, SDL_FALSE);

	m_IsFullscreen = fullscreen;

	g_xres = m_CurrentW;
	g_yres = m_CurrentH;

	return true;
}

bool CVideoMode::InitSDL()
{
	ENSURE(!m_IsInitialised);

	ReadConfig();

	EnableS3TC();

	// preferred video mode = current desktop settings
	// (command line params may override these)
	gfx::GetVideoMode(&m_PreferredW, &m_PreferredH, &m_PreferredBPP, &m_PreferredFreq);

	int w = m_ConfigW;
	int h = m_ConfigH;

	if (m_ConfigFullscreen)
	{
		// If fullscreen and no explicit size set, default to the desktop resolution
		if (w == 0 || h == 0)
		{
			w = m_PreferredW;
			h = m_PreferredH;
		}
	}

	// If no size determined, default to something sensible
	if (w == 0 || h == 0)
	{
		w = DEFAULT_WINDOW_W;
		h = DEFAULT_WINDOW_H;
	}

	if (!m_ConfigFullscreen)
	{
		// Limit the window to the screen size (if known)
		if (m_PreferredW)
			w = std::min(w, m_PreferredW);
		if (m_PreferredH)
			h = std::min(h, m_PreferredH);
	}

	int bpp = GetBestBPP();

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#if CONFIG2_GLES
	// Require GLES 2.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	if (!SetVideoMode(w, h, bpp, m_ConfigFullscreen))
	{
		// Fall back to a smaller depth buffer
		// (The rendering may be ugly but this helps when running in VMware)
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

		if (!SetVideoMode(w, h, bpp, m_ConfigFullscreen))
			return false;
	}

	SDL_GL_SetSwapInterval(g_VSync ? 1 : 0);

	// Work around a bug in the proprietary Linux ATI driver (at least versions 8.16.20 and 8.14.13).
	// The driver appears to register its own atexit hook on context creation.
	// If this atexit hook is called before SDL_Quit destroys the OpenGL context,
	// some kind of double-free problem causes a crash and lockup in the driver.
	// Calling SDL_Quit twice appears to be harmless, though, and avoids the problem
	// by destroying the context *before* the driver's atexit hook is called.
	// (Note that atexit hooks are guaranteed to be called in reverse order of their registration.)
	atexit(SDL_Quit);
	// End work around.

	ogl_Init(); // required after each mode change
	// (TODO: does that mean we need to call this when toggling fullscreen later?)

#if !OS_ANDROID
	u16 ramp[256];
	SDL_CalculateGammaRamp(g_Gamma, ramp);
	if (SDL_SetWindowGammaRamp(m_Window, ramp, ramp, ramp) < 0)
		LOGWARNING("SDL_SetWindowGammaRamp failed");
#endif

	m_IsInitialised = true;

	if (!m_ConfigFullscreen)
	{
		m_WindowedW = w;
		m_WindowedH = h;
	}

	SetWindowIcon();

	return true;
}

bool CVideoMode::InitNonSDL()
{
	ENSURE(!m_IsInitialised);

	ReadConfig();

	EnableS3TC();

	m_IsInitialised = true;

	return true;
}

void CVideoMode::Shutdown()
{
	ENSURE(m_IsInitialised);

	m_IsFullscreen = false;
	m_IsInitialised = false;
	if (m_Window)
	{
		SDL_DestroyWindow(m_Window);
		m_Window = NULL;
	}
}

void CVideoMode::EnableS3TC()
{
	// On Linux we have to try hard to get S3TC compressed texture support.
	// If the extension is already provided by default, that's fine.
	// Otherwise we should enable the 'force_s3tc_enable' environment variable
	// and (re)initialise the video system, so that Mesa provides the extension
	// (if the driver at least supports decompression).
	// (This overrides the force_s3tc_enable specified via driconf files.)
	// Otherwise we should complain to the user, and stop using compressed textures.
	//
	// Setting the environment variable causes Mesa to print an ugly message to stderr
	// ("ATTENTION: default value of option force_s3tc_enable overridden by environment."),
	// so it'd be nicer to skip that if S3TC will be supported by default,
	// but reinitialising video is a pain (and it might do weird things when fullscreen)
	// so we just unconditionally set it (unless our config file explicitly disables it).

#if !(OS_WIN || OS_MACOSX) // (assume Mesa is used for all non-Windows non-Mac platforms)
	if (m_ConfigForceS3TCEnable)
		setenv("force_s3tc_enable", "true", 0);
#endif
}

bool CVideoMode::ResizeWindow(int w, int h)
{
	ENSURE(m_IsInitialised);

	// Ignore if not windowed
	if (m_IsFullscreen)
		return true;

	// Ignore if the size hasn't changed
	if (w == m_WindowedW && h == m_WindowedH)
		return true;

	int bpp = GetBestBPP();

	if (!SetVideoMode(w, h, bpp, false))
		return false;

	m_WindowedW = w;
	m_WindowedH = h;

	UpdateRenderer(w, h);

	return true;
}

bool CVideoMode::SetFullscreen(bool fullscreen)
{
	// This might get called before initialisation by psDisplayError;
	// if so then silently fail
	if (!m_IsInitialised)
		return false;

	// Check whether this is actually a change
	if (fullscreen == m_IsFullscreen)
		return true;

	if (!m_IsFullscreen)
	{
		// Windowed -> fullscreen:

		int w = 0, h = 0;

		// If a fullscreen size was configured, use that; else use the desktop size; else use a default
		if (m_ConfigFullscreen)
		{
			w = m_ConfigW;
			h = m_ConfigH;
		}
		if (w == 0 || h == 0)
		{
			w = m_PreferredW;
			h = m_PreferredH;
		}
		if (w == 0 || h == 0)
		{
			w = DEFAULT_FULLSCREEN_W;
			h = DEFAULT_FULLSCREEN_H;
		}

		int bpp = GetBestBPP();

		if (!SetVideoMode(w, h, bpp, fullscreen))
			return false;

		UpdateRenderer(m_CurrentW, m_CurrentH);

		return true;
	}
	else
	{
		// Fullscreen -> windowed:

		// Go back to whatever the previous window size was
		int w = m_WindowedW, h = m_WindowedH;

		int bpp = GetBestBPP();

		if (!SetVideoMode(w, h, bpp, fullscreen))
			return false;

		UpdateRenderer(w, h);

		return true;
	}
}

bool CVideoMode::ToggleFullscreen()
{
	return SetFullscreen(!m_IsFullscreen);
}

void CVideoMode::UpdatePosition(int x, int y)
{
	if (!m_IsFullscreen)
	{
		m_WindowedX = x;
		m_WindowedY = y;
	}
}

void CVideoMode::UpdateRenderer(int w, int h)
{
	if (w < 2) w = 2; // avoid GL errors caused by invalid sizes
	if (h < 2) h = 2;

	g_xres = w;
	g_yres = h;

	SViewPort vp = { 0, 0, w, h };

	if (CRenderer::IsInitialised())
	{
		g_Renderer.SetViewport(vp);
		g_Renderer.Resize(w, h);
	}

	if (g_GUI)
		g_GUI->UpdateResolution();

	if (g_Console)
		g_Console->UpdateScreenSize(w, h);

	if (g_Game)
		g_Game->GetView()->SetViewport(vp);
}

int CVideoMode::GetBestBPP()
{
	if (m_ConfigBPP)
		return m_ConfigBPP;
	if (m_PreferredBPP)
		return m_PreferredBPP;
	return 32;
}

int CVideoMode::GetXRes()
{
	ENSURE(m_IsInitialised);
	return m_CurrentW;
}

int CVideoMode::GetYRes()
{
	ENSURE(m_IsInitialised);
	return m_CurrentH;
}

int CVideoMode::GetBPP()
{
	ENSURE(m_IsInitialised);
	return m_CurrentBPP;
}

int CVideoMode::GetDesktopXRes()
{
	ENSURE(m_IsInitialised);
	return m_PreferredW;
}

int CVideoMode::GetDesktopYRes()
{
	ENSURE(m_IsInitialised);
	return m_PreferredH;
}

int CVideoMode::GetDesktopBPP()
{
	ENSURE(m_IsInitialised);
	return m_PreferredBPP;
}

int CVideoMode::GetDesktopFreq()
{
	ENSURE(m_IsInitialised);
	return m_PreferredFreq;
}

SDL_Window* CVideoMode::GetWindow()
{
	ENSURE(m_IsInitialised);
	return m_Window;
}

void CVideoMode::SetWindowIcon()
{
	std::shared_ptr<u8> iconFile;
	size_t iconFileSize;
	if (g_VFS->LoadFile("art/textures/icons/window.png", iconFile, iconFileSize) != INFO::OK)
	{
		LOGWARNING("Window icon not found.");
		return;
	}

	Tex iconTexture;
	if (iconTexture.decode(iconFile, iconFileSize) != INFO::OK)
		return;

	// Convert to required BGRA format.
	const size_t iconFlags = (iconTexture.m_Flags | TEX_BGR) & ~TEX_DXT;
	if (iconTexture.transform_to(iconFlags) != INFO::OK)
		return;

	void* bgra_img = iconTexture.get_data();
	if (!bgra_img)
		return;

	SDL_Surface *iconSurface = SDL_CreateRGBSurfaceFrom(bgra_img,
		iconTexture.m_Width, iconTexture.m_Height, 32, iconTexture.m_Width * 4,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (!iconSurface)
		return;

	SDL_SetWindowIcon(m_Window, iconSurface);
	SDL_FreeSurface(iconSurface);
}
