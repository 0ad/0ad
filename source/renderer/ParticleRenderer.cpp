/* Copyright (C) 2011 Wildfire Games.
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

#include "ParticleRenderer.h"

#include "graphics/ParticleEmitter.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"

struct ParticleRendererInternals
{
	CShaderProgramPtr shader;
	CShaderProgramPtr shaderSolid;
	std::vector<CParticleEmitter*> emitters;
};

ParticleRenderer::ParticleRenderer()
{
	m = new ParticleRendererInternals();
}

ParticleRenderer::~ParticleRenderer()
{
	delete m;
}

void ParticleRenderer::Submit(CParticleEmitter* emitter)
{
	m->emitters.push_back(emitter);
}

void ParticleRenderer::EndFrame()
{
	m->emitters.clear();
	// this should leave the capacity unchanged, which is okay since it
	// won't be very large or very variable
}

struct SortEmitterDistance
{
	SortEmitterDistance(const CMatrix3D& m) : worldToCam(m) { }

	// TODO: if this is slow, we should pre-compute the distance for each emitter

	bool operator()(CParticleEmitter* const& a, CParticleEmitter* const& b)
	{
		CVector3D posa = a->GetPosition();
		CVector3D posb = b->GetPosition();
		if (posa == posb)
			return false;
		float dista = worldToCam.Transform(posa).LengthSquared();
		float distb = worldToCam.Transform(posb).LengthSquared();
		return distb < dista;
	}

	CMatrix3D worldToCam;
};

void ParticleRenderer::PrepareForRendering()
{
	PROFILE3("prepare particles");

#if CONFIG2_GLES
#warning TODO: implement particles for GLES
	return;
#endif

	// Can't load the shader in the constructor because it's called before the
	// renderer initialisation is complete, so load it the first time through here
	if (!m->shader)
	{
		// Only construct the shaders when shaders are supported and enabled; otherwise
		// RenderParticles will never be called so it's safe to leave the shaders as null
		if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		{
			m->shader = g_Renderer.GetShaderManager().LoadProgram("particle");
			m->shaderSolid = g_Renderer.GetShaderManager().LoadProgram("particle_solid");
		}
	}

	{
		PROFILE("update emitters");
		for (size_t i = 0; i < m->emitters.size(); ++i)
		{
			CParticleEmitter* emitter = m->emitters[i];
			emitter->UpdateArrayData();
		}
	}

	{
		// Sort back-to-front by distance from camera
		PROFILE("sort emitters");
		CMatrix3D worldToCam;
		g_Renderer.GetViewCamera().m_Orientation.GetInverse(worldToCam);
		std::stable_sort(m->emitters.begin(), m->emitters.end(), SortEmitterDistance(worldToCam));
	}

	// TODO: should batch by texture here when possible, maybe
}

void ParticleRenderer::RenderParticles(bool solidColor)
{
#if CONFIG2_GLES
#warning TODO: implement particles for GLES
	return;
#endif

	CShaderProgramPtr shader = solidColor ? m->shaderSolid : m->shader;

	shader->Bind();

	if (!solidColor)
		glEnable(GL_BLEND);
	glDepthMask(0);

	for (size_t i = 0; i < m->emitters.size(); ++i)
	{
		CParticleEmitter* emitter = m->emitters[i];

		emitter->Bind();
		emitter->RenderArray(shader);
	}

	CVertexBuffer::Unbind();

	pglBlendEquationEXT(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_BLEND);
	glDepthMask(1);

	shader->Unbind();
}

void ParticleRenderer::RenderBounds(CShaderProgramPtr& shader)
{
	for (size_t i = 0; i < m->emitters.size(); ++i)
	{
		CParticleEmitter* emitter = m->emitters[i];

		CBoundingBoxAligned bounds = emitter->m_Type->CalculateBounds(emitter->GetPosition(), emitter->GetParticleBounds());
		bounds.Render(shader);
	}
}
