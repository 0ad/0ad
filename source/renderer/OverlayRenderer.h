/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_OVERLAYRENDERER
#define INCLUDED_OVERLAYRENDERER

struct SOverlayLine;
struct SOverlaySprite;
class CCamera;

struct OverlayRendererInternals;

/**
 * Class OverlayRenderer: Render various bits of data that overlay the
 * game world (selection circles, health bars, etc).
 */
class OverlayRenderer
{
public:
	OverlayRenderer();
	~OverlayRenderer();

	/**
	 * Add a line overlay for rendering in this frame.
	 */
	void Submit(SOverlayLine* overlay);

	/**
	 * Add a sprite overlay for rendering in this frame.
	 */
	void Submit(SOverlaySprite* overlay);

	/**
	 * Prepare internal data structures for rendering.
	 * Must be called after all Submit calls for a frame, and before
	 * any rendering calls.
	 */
	void PrepareForRendering();

	/**
	 * Reset the list of submitted overlays.
	 */
	void EndFrame();

	/**
	 * Render all the submitted overlays that are embedded in the world
	 * (i.e. rendered behind other objects, underwater, etc).
	 */
	void RenderOverlays();

	/**
	 * Render all the submitted overlays that should appear on top of everything
	 * in the world.
	 * @param viewCamera camera to be used for billboard computations
	 */
	void RenderForegroundOverlays(const CCamera& viewCamera);

private:
	OverlayRendererInternals* m;
};

#endif // INCLUDED_OVERLAYRENDERER
