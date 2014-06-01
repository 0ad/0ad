/* Copyright (C) 2013 Wildfire Games.
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

#include "graphics/ShaderManager.h"

struct SOverlayLine;
struct SOverlayTexturedLine;
struct SOverlaySprite;
struct SOverlayQuad;
struct SOverlaySphere;
class CCamera;

struct OverlayRendererInternals;

/**
 * Class OverlayRenderer: Render various bits of data that overlay the
 * game world (selection circles, health bars, etc).
 */
class OverlayRenderer
{
	NONCOPYABLE(OverlayRenderer);

public:
	OverlayRenderer();
	~OverlayRenderer();

	/**
	 * Performs one-time initialization. Called by CRenderer::Open after graphics
	 * capabilities and the shader path have been determined (notably VBO support).
	 */
	void Initialize();

	/**
	 * Add a line overlay for rendering in this frame.
	 * @param overlay Must be non-null. The pointed-to object must remain valid at least
	 *                until the end of the frame.
	 */
	void Submit(SOverlayLine* overlay);

	/**
	 * Add a textured line overlay for rendering in this frame.
	 * @param overlay Must be non-null. The pointed-to object must remain valid at least
	 *                until the end of the frame.
	 */
	void Submit(SOverlayTexturedLine* overlay);

	/**
	 * Add a sprite overlay for rendering in this frame.
	 * @param overlay Must be non-null. The pointed-to object must remain valid at least
	 *                until the end of the frame.
	 */
	void Submit(SOverlaySprite* overlay);

	/**
	 * Add a textured quad overlay for rendering in this frame.
	 * @param overlay Must be non-null. The pointed-to object must remain valid at least
	 *                until the end of the frame.
	 */
	void Submit(SOverlayQuad* overlay);

	/**
	 * Add a sphere overlay for rendering in this frame.
	 * @param overlay Must be non-null. The pointed-to object must remain valid at least
	 *                until the end of the frame.
	 */
	void Submit(SOverlaySphere* overlay);

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
	 * (i.e. rendered behind other objects in the normal 3D way)
	 * and should be drawn before water (i.e. may be visible under the water)
	 */
	void RenderOverlaysBeforeWater();

	/**
	 * Render all the submitted overlays that are embedded in the world
	 * (i.e. rendered behind other objects in the normal 3D way)
	 * and should be drawn after water (i.e. may be visible on top of the water)
	 */
	void RenderOverlaysAfterWater();

	/**
	 * Render all the submitted overlays that should appear on top of everything
	 * in the world.
	 * @param viewCamera camera to be used for billboard computations
	 */
	void RenderForegroundOverlays(const CCamera& viewCamera);

	/// Small vertical offset of overlays from terrain to prevent visual glitches
	static const float OVERLAY_VOFFSET;

private:
	
	/**
	 * Helper method; renders all overlay lines currently registered in the internals. Batch-
	 * renders textured overlay lines batched according to their visibility status by delegating
	 * to RenderTexturedOverlayLines(CShaderProgramPtr, bool).
	 */
	void RenderTexturedOverlayLines();

	/**
	 * Helper method; renders those overlay lines currently registered in the internals (i.e.
	 * in m->texlines) for which the 'always visible' flag equals @p alwaysVisible. Used for
	 * batch rendering the overlay lines according to their alwaysVisible status, as this
	 * requires a separate shader to be used.
	 */
	void RenderTexturedOverlayLines(CShaderProgramPtr shader, bool alwaysVisible);

	/**
	 * Helper method; batch-renders all registered quad overlays, batched by their texture for effiency.
	 */
	void RenderQuadOverlays();

	/**
	 * Helper method; batch-renders all sphere quad overlays.
	 */
	 void RenderSphereOverlays();

private:
	OverlayRendererInternals* m;
};

#endif // INCLUDED_OVERLAYRENDERER
