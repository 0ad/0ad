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

#ifndef INCLUDED_PARTICLERENDERER
#define INCLUDED_PARTICLERENDERER

class CParticleEmitter;
class CShaderDefines;
struct ParticleRendererInternals;

#include "graphics/ShaderProgramPtr.h"

/**
 * Render particles.
 */
class ParticleRenderer
{
	NONCOPYABLE(ParticleRenderer);

public:
	ParticleRenderer();
	~ParticleRenderer();

	/**
	 * Add an emitter for rendering in this frame.
	 */
	void Submit(int cullGroup, CParticleEmitter* emitter);

	/**
	 * Prepare internal data structures for rendering.
	 * Must be called after all Submit calls for a frame, and before
	 * any rendering calls.
	 */
	void PrepareForRendering(const CShaderDefines& context);

	/**
	 * Reset the list of submitted overlays.
	 */
	void EndFrame();

	/**
	 * Render all the submitted particles.
	 */
	void RenderParticles(int cullGroup, bool solidColor = false);

	/**
	 * Render bounding boxes for all the submitted emitters.
	 */
	void RenderBounds(int cullGroup, CShaderProgramPtr& shader);

private:
	ParticleRendererInternals* m;
};

#endif // INCLUDED_PARTICLERENDERER
