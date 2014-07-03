/* Copyright (C) 2012 Wildfire Games.
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

#include "ParticleEmitter.h"

#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/ParticleEmitterType.h"
#include "graphics/ParticleManager.h"
#include "graphics/ShaderProgram.h"
#include "graphics/TextureManager.h"

#include "renderer/Renderer.h"

CParticleEmitter::CParticleEmitter(const CParticleEmitterTypePtr& type) :
	m_Type(type), m_Active(true), m_NextParticleIdx(0), m_EmissionRoundingError(0.f),
	m_LastUpdateTime(type->m_Manager.GetCurrentTime()),
	m_IndexArray(GL_DYNAMIC_DRAW),
	m_VertexArray(GL_DYNAMIC_DRAW),
	m_LastFrameNumber(-1)
{
	// If we should start with particles fully emitted, pretend that we
	// were created in the past so the first update will produce lots of
	// particles.
	// TODO: instead of this, maybe it would make more sense to do a full
	// lifetime-length update of all emitters when the game first starts
	// (so that e.g. buildings constructed later on won't have fully-started
	// emitters, but those at the start will)?
	if (m_Type->m_StartFull)
		m_LastUpdateTime -= m_Type->m_MaxLifetime;

	m_Particles.reserve(m_Type->m_MaxParticles);

	m_AttributePos.type = GL_FLOAT;
	m_AttributePos.elems = 3;
	m_VertexArray.AddAttribute(&m_AttributePos);

	m_AttributeAxis.type = GL_FLOAT;
	m_AttributeAxis.elems = 2;
	m_VertexArray.AddAttribute(&m_AttributeAxis);

	m_AttributeUV.type = GL_FLOAT;
	m_AttributeUV.elems = 2;
	m_VertexArray.AddAttribute(&m_AttributeUV);

	m_AttributeColor.type = GL_UNSIGNED_BYTE;
	m_AttributeColor.elems = 4;
	m_VertexArray.AddAttribute(&m_AttributeColor);

	m_VertexArray.SetNumVertices(m_Type->m_MaxParticles * 4);
	m_VertexArray.Layout();

	m_IndexArray.SetNumVertices(m_Type->m_MaxParticles * 6);
	m_IndexArray.Layout();
	VertexArrayIterator<u16> index = m_IndexArray.GetIterator();
	for (size_t i = 0; i < m_Type->m_MaxParticles; ++i)
	{
		*index++ = i*4 + 0;
		*index++ = i*4 + 1;
		*index++ = i*4 + 2;
		*index++ = i*4 + 2;
		*index++ = i*4 + 3;
		*index++ = i*4 + 0;
	}
	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();
}

void CParticleEmitter::UpdateArrayData(int frameNumber)
{
	if (m_LastFrameNumber == frameNumber)
		return;

	m_LastFrameNumber = frameNumber;

	// Update m_Particles
	m_Type->UpdateEmitter(*this, m_Type->m_Manager.GetCurrentTime() - m_LastUpdateTime);
	m_LastUpdateTime = m_Type->m_Manager.GetCurrentTime();

	// Regenerate the vertex array data:

	VertexArrayIterator<CVector3D> attrPos = m_AttributePos.GetIterator<CVector3D>();
	VertexArrayIterator<float[2]> attrAxis = m_AttributeAxis.GetIterator<float[2]>();
	VertexArrayIterator<float[2]> attrUV = m_AttributeUV.GetIterator<float[2]>();
	VertexArrayIterator<SColor4ub> attrColor = m_AttributeColor.GetIterator<SColor4ub>();

	ENSURE(m_Particles.size() <= m_Type->m_MaxParticles);

	CBoundingBoxAligned bounds;

	for (size_t i = 0; i < m_Particles.size(); ++i)
	{
		// TODO: for more efficient rendering, maybe we should replace this with
		// a degenerate quad if alpha is 0

		bounds += m_Particles[i].pos;

		*attrPos++ = m_Particles[i].pos;
		*attrPos++ = m_Particles[i].pos;
		*attrPos++ = m_Particles[i].pos;
		*attrPos++ = m_Particles[i].pos;

		// Compute corner offsets, split into sin/cos components so the vertex
		// shader can multiply by the camera-right (or left?) and camera-up vectors
		// to get rotating billboards:

		float s = sin(m_Particles[i].angle) * m_Particles[i].size/2.f;
		float c = cos(m_Particles[i].angle) * m_Particles[i].size/2.f;

		(*attrAxis)[0] = c;
		(*attrAxis)[1] = s;
		++attrAxis;
		(*attrAxis)[0] = s;
		(*attrAxis)[1] = -c;
		++attrAxis;
		(*attrAxis)[0] = -c;
		(*attrAxis)[1] = -s;
		++attrAxis;
		(*attrAxis)[0] = -s;
		(*attrAxis)[1] = c;
		++attrAxis;

		(*attrUV)[0] = 1;
		(*attrUV)[1] = 0;
		++attrUV;
		(*attrUV)[0] = 0;
		(*attrUV)[1] = 0;
		++attrUV;
		(*attrUV)[0] = 0;
		(*attrUV)[1] = 1;
		++attrUV;
		(*attrUV)[0] = 1;
		(*attrUV)[1] = 1;
		++attrUV;

		SColor4ub color = m_Particles[i].color;

		// Special case: If the blending depends on the source colour, not the source alpha,
		// then pre-multiply by the alpha. (This is kind of a hack.)
		if (m_Type->m_BlendFuncDst == GL_ONE_MINUS_SRC_COLOR)
		{
			color.R = (color.R * color.A) / 255;
			color.G = (color.G * color.A) / 255;
			color.B = (color.B * color.A) / 255;
		}

		*attrColor++ = color;
		*attrColor++ = color;
		*attrColor++ = color;
		*attrColor++ = color;
	}

	m_ParticleBounds = bounds;

	m_VertexArray.Upload();
}

void CParticleEmitter::Bind(const CShaderProgramPtr& shader)
{
	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
	shader->BindTexture(str_losTex, los.GetTextureSmooth());
	shader->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);
	
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	shader->Uniform(str_sunColor, lightEnv.m_SunColor);
	shader->Uniform(str_fogColor, lightEnv.m_FogColor);
	shader->Uniform(str_fogParams, lightEnv.m_FogFactor, lightEnv.m_FogMax, 0.f, 0.f);

	shader->BindTexture(str_baseTex, m_Type->m_Texture);
	pglBlendEquationEXT(m_Type->m_BlendEquation);
	glBlendFunc(m_Type->m_BlendFuncSrc, m_Type->m_BlendFuncDst);
}

void CParticleEmitter::RenderArray(const CShaderProgramPtr& shader)
{
	// Some drivers apparently don't like count=0 in glDrawArrays here,
	// so skip all drawing in that case
	if (m_Particles.empty())
		return;

	u8* indexBase = m_IndexArray.Bind();
	u8* base = m_VertexArray.Bind();

	GLsizei stride = (GLsizei)m_VertexArray.GetStride();

	shader->VertexPointer(3, GL_FLOAT, stride, base + m_AttributePos.offset);

	// Pass the sin/cos axis components as texcoords for no particular reason
	// other than that they fit. (Maybe this should be glVertexAttrib* instead?)
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, base + m_AttributeUV.offset);
	shader->TexCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, stride, base + m_AttributeAxis.offset);

	shader->ColorPointer(4, GL_UNSIGNED_BYTE, stride, base + m_AttributeColor.offset);

	shader->AssertPointersBound();
	glDrawElements(GL_TRIANGLES, (GLsizei)(m_Particles.size() * 6), GL_UNSIGNED_SHORT, indexBase);

	g_Renderer.GetStats().m_DrawCalls++;
	g_Renderer.GetStats().m_Particles += m_Particles.size();
}

void CParticleEmitter::Unattach(const CParticleEmitterPtr& self)
{
	m_Active = false;
	m_Type->m_Manager.AddUnattachedEmitter(self);
}

void CParticleEmitter::AddParticle(const SParticle& particle)
{
	if (m_NextParticleIdx >= m_Particles.size())
		m_Particles.push_back(particle);
	else
		m_Particles[m_NextParticleIdx] = particle;

	m_NextParticleIdx = (m_NextParticleIdx + 1) % m_Type->m_MaxParticles;
}

void CParticleEmitter::SetEntityVariable(const std::string& name, float value)
{
	m_EntityVariables[name] = value;
}



CModelParticleEmitter::CModelParticleEmitter(const CParticleEmitterTypePtr& type) :
	m_Type(type)
{
	m_Emitter = CParticleEmitterPtr(new CParticleEmitter(m_Type));
}

CModelParticleEmitter::~CModelParticleEmitter()
{
	m_Emitter->Unattach(m_Emitter);
}

void CModelParticleEmitter::SetEntityVariable(const std::string& name, float value)
{
	m_Emitter->SetEntityVariable(name, value);
}

CModelAbstract* CModelParticleEmitter::Clone() const
{
	return new CModelParticleEmitter(m_Type);
}

void CModelParticleEmitter::CalcBounds()
{
	// TODO: we ought to compute sensible bounds here, probably based on the
	// current computed particle positions plus the emitter type's largest
	// potential bounding box at the current position

	m_WorldBounds = m_Type->CalculateBounds(m_Emitter->GetPosition(), m_Emitter->GetParticleBounds());
}

void CModelParticleEmitter::ValidatePosition()
{
	// TODO: do we need to do anything here?

	// This is a convenient (though possibly not particularly appropriate) place
	// to invalidate bounds so they'll be recomputed from the recent particle data
	InvalidateBounds();
}

void CModelParticleEmitter::InvalidatePosition()
{
}

void CModelParticleEmitter::SetTransform(const CMatrix3D& transform)
{
	m_Emitter->SetPosition(transform.GetTranslation());
	m_Emitter->SetRotation(transform.GetRotation());
}
