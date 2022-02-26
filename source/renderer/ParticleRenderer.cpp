/* Copyright (C) 2022 Wildfire Games.
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
#include "graphics/ShaderDefines.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include "ps/CStrInternStatic.h"
#include "ps/Profile.h"
#include "renderer/DebugRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"

struct ParticleRendererInternals
{
	int frameNumber;
	CShaderTechniquePtr techAdd;
	CShaderTechniquePtr techSubtract;
	CShaderTechniquePtr techOverlay;
	CShaderTechniquePtr techMultiply;
	CShaderTechniquePtr techWireframe;
	std::vector<CParticleEmitter*> emitters[CSceneRenderer::CULL_MAX];
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
	for (int cullGroup = 0; cullGroup < CSceneRenderer::CULL_MAX; ++cullGroup)
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
	if (!m->techWireframe)
	{
		m->techAdd = g_Renderer.GetShaderManager().LoadEffect(str_particle_add, context);
		m->techSubtract = g_Renderer.GetShaderManager().LoadEffect(str_particle_subtract, context);
		m->techOverlay = g_Renderer.GetShaderManager().LoadEffect(str_particle_overlay, context);
		m->techMultiply = g_Renderer.GetShaderManager().LoadEffect(str_particle_multiply, context);
		CShaderDefines contextWithWireframe = context;
		contextWithWireframe.Add(str_MODE_WIREFRAME, str_1);
		m->techWireframe = g_Renderer.GetShaderManager().LoadEffect(str_particle_solid, contextWithWireframe);
	}

	++m->frameNumber;

	for (int cullGroup = 0; cullGroup < CSceneRenderer::CULL_MAX; ++cullGroup)
	{
		PROFILE("update emitters");
		for (size_t i = 0; i < m->emitters[cullGroup].size(); ++i)
		{
			CParticleEmitter* emitter = m->emitters[cullGroup][i];
			emitter->UpdateArrayData(m->frameNumber);
			emitter->PrepareForRendering();
		}
	}

	for (int cullGroup = 0; cullGroup < CSceneRenderer::CULL_MAX; ++cullGroup)
	{
		// Sort back-to-front by distance from camera
		PROFILE("sort emitters");
		CMatrix3D worldToCam;
		g_Renderer.GetSceneRenderer().GetViewCamera().GetOrientation().GetInverse(worldToCam);
		std::stable_sort(m->emitters[cullGroup].begin(), m->emitters[cullGroup].end(), SortEmitterDistance(worldToCam));
	}

	// TODO: should batch by texture here when possible, maybe
}

void ParticleRenderer::RenderParticles(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	int cullGroup, bool wireframe)
{
	CShaderTechnique* lastTech = nullptr;
	for (CParticleEmitter* emitter : m->emitters[cullGroup])
	{
		CShaderTechnique* currentTech = nullptr;
		if (wireframe)
		{
			currentTech = m->techWireframe.get();
		}
		else
		{
			switch (emitter->m_Type->m_BlendMode)
			{
			case CParticleEmitterType::BlendMode::ADD: currentTech = m->techAdd.get(); break;
			case CParticleEmitterType::BlendMode::SUBTRACT: currentTech = m->techSubtract.get(); break;
			case CParticleEmitterType::BlendMode::OVERLAY: currentTech = m->techOverlay.get(); break;
			case CParticleEmitterType::BlendMode::MULTIPLY: currentTech = m->techMultiply.get(); break;
			}
		}
		ENSURE(currentTech);
		if (lastTech != currentTech)
		{
			lastTech = currentTech;
			lastTech->BeginPass();
			deviceCommandContext->SetGraphicsPipelineState(lastTech->GetGraphicsPipelineStateDesc());

			const CShaderProgramPtr& shader = lastTech->GetShader();
			shader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
			shader->Uniform(str_modelViewMatrix, g_Renderer.GetSceneRenderer().GetViewCamera().GetOrientation().GetInverse());
		}
		emitter->Bind(deviceCommandContext, lastTech->GetShader());
		emitter->RenderArray(deviceCommandContext, lastTech->GetShader());
	}

	if (lastTech)
		lastTech->EndPass();

	CVertexBuffer::Unbind(deviceCommandContext);
}

void ParticleRenderer::RenderBounds(int cullGroup)
{
	for (const CParticleEmitter* emitter : m->emitters[cullGroup])
	{
		const CBoundingBoxAligned bounds =
			emitter->m_Type->CalculateBounds(emitter->GetPosition(), emitter->GetParticleBounds());
		g_Renderer.GetDebugRenderer().DrawBoundingBox(bounds, CColor(0.0f, 1.0f, 0.0f, 1.0f), true);
	}
}
