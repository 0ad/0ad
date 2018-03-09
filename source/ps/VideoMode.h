/* Copyright (C) 2018 Wildfire Games.
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

typedef struct SDL_Window SDL_Window;

class CVideoMode
{
public:
	CVideoMode();

	/**
	 * Initialise the video mode, for use in an SDL-using application.
	 */
	bool InitSDL();

	/**
	 * Initialise parts of the video mode, for use in Atlas (which uses
	 * wxWidgets instead of SDL for GL).
	 * Currently this just tries to enable S3TC.
	 */
	bool InitNonSDL();

	/**
	 * Shut down after InitSDL/InitNonSDL, so that they can be used again.
	 */
	void Shutdown();

	/**
	 * Resize the SDL window and associated graphics stuff to the new size.
	 */
	bool ResizeWindow(int w, int h);

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

	int GetXRes();
	int GetYRes();
	int GetBPP();

	int GetDesktopXRes();
	int GetDesktopYRes();
	int GetDesktopBPP();
	int GetDesktopFreq();

	SDL_Window* GetWindow();

	void SetWindowIcon();

private:
	void ReadConfig();
	int GetBestBPP();
	bool SetVideoMode(int w, int h, int bpp, bool fullscreen);
	void EnableS3TC();

	/**
	 * Remember whether Init has been called. (This isn't used for anything
	 * important, just for verifying that the callers call our methods in
	 * the right order.)
	 */
	bool m_IsInitialised;

	SDL_Window* m_Window;

	// Initial desktop settings
	int m_PreferredW;
	int m_PreferredH;
	int m_PreferredBPP;
	int m_PreferredFreq;

	// Config file settings (0 if unspecified)
	int m_ConfigW;
	int m_ConfigH;
	int m_ConfigBPP;
	int m_ConfigDisplay;
	bool m_ConfigFullscreen;
	bool m_ConfigForceS3TCEnable;

	// If we're fullscreen, size/position of window when we were last windowed (or the default window
	// size/position if we started fullscreen), to support switching back to the old window size/position
	int m_WindowedW;
	int m_WindowedH;
	int m_WindowedX;
	int m_WindowedY;

	// Whether we're currently being displayed fullscreen
	bool m_IsFullscreen;

	// The last mode selected
	int m_CurrentW;
	int m_CurrentH;
	int m_CurrentBPP;
};

extern CVideoMode g_VideoMode;

#endif // INCLUDED_VIDEOMODE
