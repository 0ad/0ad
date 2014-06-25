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
	int frameNumber;
	CShaderTechniquePtr shader;
	CShaderTechniquePtr shaderSolid;
	std::vector<CParticleEmitter*> emitters[CRenderer::CULL_MAX];
};

ParticleRenderer::ParticleRenderer()
{
	m = new ParticleRendererInternals();
	m->frameNumber = 0;
}

ParticleRenderer::~ParticleRenderer()
{
	delete m;
}

void ParticleRenderer::Submit(int cullGroup, CParticleEmitter* emitter)
{
	m->emitters[cullGroup].push_back(emitter);
}

void ParticleRenderer::EndFrame()
{
	for (int cullGroup = 0; cullGroup < CRenderer::CULL_MAX; ++cullGroup)
		m->emitters[cullGroup].clear();
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

void ParticleRenderer::PrepareForRendering(const CShaderDefines& context)
{
	PROFILE3("prepare particles");

	// Can't load the shader in the constructor because it's called before the
	// renderer initialisation is complete, so load it the first time through here
	if (!m->shader)
	{
		// Only construct the shaders when shaders are supported and enabled; otherwise
		// RenderParticles will never be called so it's safe to leave the shaders as null
		if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		{
			m->shader = g_Renderer.GetShaderManager().LoadEffect(str_particle, context, CShaderDefines());
			m->shaderSolid = g_Renderer.GetShaderManager().LoadEffect(str_particle_solid, context, CShaderDefines());
		}
	}

	++m->frameNumber;

	for (int cullGroup = 0; cullGroup < CRenderer::CULL_MAX; ++cullGroup)
	{
		PROFILE("update emitters");
		for (size_t i = 0; i < m->emitters[cullGroup].size(); ++i)
		{
			CParticleEmitter* emitter = m->emitters[cullGroup][i];
			emitter->UpdateArrayData(m->frameNumber);
		}
	}

	for (int cullGroup = 0; cullGroup < CRenderer::CULL_MAX; ++cullGroup)
	{
		// Sort back-to-front by distance from camera
		PROFILE("sort emitters");
		CMatrix3D worldToCam;
		g_Renderer.GetViewCamera().m_Orientation.GetInverse(worldToCam);
		std::stable_sort(m->emitters[cullGroup].begin(), m->emitters[cullGroup].end(), SortEmitterDistance(worldToCam));
	}

	// TODO: should batch by texture here when possible, maybe
}

void ParticleRenderer::RenderParticles(int cullGroup, bool solidColor)
{
	CShaderTechniquePtr shader = solidColor ? m->shaderSolid : m->shader;

	std::vector<CParticleEmitter*>& emitters = m->emitters[cullGroup];

	shader->BeginPass();

	shader->GetShader()->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection());

	if (!solidColor)
		glEnable(GL_BLEND);
	glDepthMask(0);

	for (size_t i = 0; i < emitters.size(); ++i)
	{
		CParticleEmitter* emitter = emitters[i];

		emitter->Bind(shader->GetShader());
		emitter->RenderArray(shader->GetShader());
	}

	CVertexBuffer::Unbind();

	pglBlendEquationEXT(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_BLEND);
	glDepthMask(1);

	shader->EndPass();
}

void ParticleRenderer::RenderBounds(int cullGroup, CShaderProgramPtr& shader)
{
	std::vector<CParticleEmitter*>& emitters = m->emitters[cullGroup];

	for (size_t i = 0; i < emitters.size(); ++i)
	{
		CParticleEmitter* emitter = emitters[i];

		CBoundingBoxAligned bounds = emitter->m_Type->CalculateBounds(emitter->GetPosition(), emitter->GetParticleBounds());
		bounds.Render(shader);
	}
}
