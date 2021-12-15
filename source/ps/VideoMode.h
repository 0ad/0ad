/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_VIDEOMODE
#define INCLUDED_VIDEOMODE

#include "ps/CStrForward.h"

#include <memory>

typedef struct SDL_Window SDL_Window;

namespace Renderer
{
namespace Backend
{
namespace GL
{
class CDevice;
}
}
}

class CVideoMode
{
public:
	enum class Backend
	{
		GL,
		GL_ARB
	};

	CVideoMode();
	~CVideoMode();

	/**
	 * Initialise the video mode, for use in an SDL-using application.
	 */
	bool InitSDL();

	/**
	 * Initialise parts of the video mode, for use in Atlas (which uses
	 * wxWidgets instead of SDL for GL).
	 */
	bool InitNonSDL();

	/**
	 * Shut down after InitSDL/InitNonSDL, so that they can be used again.
	 */
	void Shutdown();

	/**
	 * Creates a backend device. Also we use wxWidgets in Atlas so we don't need
	 * to create one for that case.
	 */
	bool CreateBackendDevice(const bool createSDLContext);

	/**
	 * Resize the SDL window and associated graphics stuff to the new size.
	 */
	bool ResizeWindow(int w, int h);

	/**
	 * Set scale and tell dependent compoenent to recompute sizes.
	 */
	void Rescale(float scale);

	/**
	 * Switch to fullscreen or windowed mode.
	 */
	bool SetFullscreen(bool fullscreen);

	/**
	* Returns true if window runs in fullscreen mode.
	*/
	bool IsInFullscreen() const;

	/**
	 * Switch between fullscreen and windowed mode.
	 */
	bool ToggleFullscreen();

	/**
	 * Update window position, to restore later if necessary (SDL2 only).
	 */
	void UpdatePosition(int x, int y);

	/**
	 * Update the graphics code to start drawing to the new size.
	 * This should be called after the GL context has been resized.
	 * This can also be used when the GL context is managed externally, not via SDL.
	 */
	static void UpdateRenderer(int w, int h);

	int GetXRes() const;
	int GetYRes() const;
	int GetBPP() const;

	bool IsVSyncEnabled() const;

	int GetDesktopXRes() const;
	int GetDesktopYRes() const;
	int GetDesktopBPP() const;
	int GetDesktopFreq() const;

	float GetScale() const;

	SDL_Window* GetWindow();

	void SetWindowIcon();

	void SetCursor(const CStrW& name);
	void ResetCursor();

	Backend GetBackend() const { return m_Backend; }

	Renderer::Backend::GL::CDevice* GetBackendDevice() { return m_BackendDevice.get(); }

private:
	void ReadConfig();
	int GetBestBPP();
	bool SetVideoMode(int w, int h, int bpp, bool fullscreen);

	/**
	 * Remember whether Init has been called. (This isn't used for anything
	 * important, just for verifying that the callers call our methods in
	 * the right order.)
	 */
	bool m_IsInitialised = false;

	SDL_Window* m_Window = nullptr;

	// Initial desktop settings.
	// Frequency is in Hz, and BPP means bits per pixels (not bytes per pixels).
	int m_PreferredW = 0;
	int m_PreferredH = 0;
	int m_PreferredBPP = 0;
	int m_PreferredFreq = 0;

	float m_Scale = 1.0f;

	// Config file settings (0 if unspecified)
	int m_ConfigW = 0;
	int m_ConfigH = 0;
	int m_ConfigBPP = 0;
	int m_ConfigDisplay = 0;
	bool m_ConfigEnableHiDPI = false;
	bool m_ConfigVSync = false;

	// (m_ConfigFullscreen defaults to false, so users don't get stuck if
	// e.g. half the filesystem is missing and the config files aren't loaded).
	bool m_ConfigFullscreen = false;

	// If we're fullscreen, size/position of window when we were last windowed (or the default window
	// size/position if we started fullscreen), to support switching back to the old window size/position
	int m_WindowedW;
	int m_WindowedH;
	int m_WindowedX;
	int m_WindowedY;

	// Whether we're currently being displayed fullscreen
	bool m_IsFullscreen = false;

	// The last mode selected
	int m_CurrentW;
	int m_CurrentH;
	int m_CurrentBPP;

	class CCursor;
	std::unique_ptr<CCursor> m_Cursor;

	Backend m_Backend = Backend::GL;
	std::unique_ptr<Renderer::Backend::GL::CDevice> m_BackendDevice;
};

extern CVideoMode g_VideoMode;

#endif // INCLUDED_VIDEOMODE
