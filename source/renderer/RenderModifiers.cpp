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

/*
 * Implementation of common RenderModifiers
 */

#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "maths/Matrix3D.h"

#include "ps/Game.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Model.h"
#include "graphics/TextureManager.h"

#include "renderer/RenderModifiers.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"

#include <boost/algorithm/string.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////
// RenderModifier implementation

void RenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
}

CShaderProgramPtr RenderModifier::GetShader(int UNUSED(pass))
{
	return CShaderProgramPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////
// LitRenderModifier implementation

LitRenderModifier::LitRenderModifier()
	: m_Shadow(0), m_LightEnv(0)
{
}

LitRenderModifier::~LitRenderModifier()
{
}

// Set the shadow map for subsequent rendering
void LitRenderModifier::SetShadowMap(const ShadowMap* shadow)
{
	m_Shadow = shadow;
}

// Set the light environment for subsequent rendering
void LitRenderModifier::SetLightEnv(const CLightEnv* lightenv)
{
	m_LightEnv = lightenv;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// PlainRenderModifier implementation

PlainRenderModifier::PlainRenderModifier()
{
}

PlainRenderModifier::~PlainRenderModifier()
{
}

int PlainRenderModifier::BeginPass(int pass)
{
	ENSURE(pass == 0);

	// set up texture environment for base pass - modulate texture and primary color
	pglActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, g_Renderer.m_Options.m_LodBias);

	// pass one through as alpha; transparent textures handled specially by TransparencyRenderer
	// (gl_constant means the colour comes from the gl_texture_env_color)
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	float color[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	return STREAM_POS|STREAM_COLOR|STREAM_UV0;
}

bool PlainRenderModifier::EndPass(int UNUSED(pass))
{
	// We didn't modify blend state or higher texenvs, so we don't have
	// to reset OpenGL state here.

	return true;
}

void PlainRenderModifier::PrepareTexture(int UNUSED(pass), CTexturePtr& texture)
{
	texture->Bind(0);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// WireframeRenderModifier implementation


WireframeRenderModifier::WireframeRenderModifier()
{
}

WireframeRenderModifier::~WireframeRenderModifier()
{
}

int WireframeRenderModifier::BeginPass(int pass)
{
	ENSURE(pass == 0);

	// first switch on wireframe
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// setup some renderstate ..
	glDepthMask(0);
	ogl_tex_bind(0, 0);
	glColor4f(1,1,1,0.75f);
	glLineWidth(1.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	return STREAM_POS;
}


bool WireframeRenderModifier::EndPass(int UNUSED(pass))
{
	// .. restore the renderstates
	glDisable(GL_BLEND);
	glDepthMask(1);

	// restore fill mode, and we're done
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	return true;
}


void WireframeRenderModifier::PrepareTexture(int UNUSED(pass), CTexturePtr& UNUSED(texture))
{
}


void WireframeRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
}



///////////////////////////////////////////////////////////////////////////////////////////////
// SolidColorRenderModifier implementation

SolidColorRenderModifier::SolidColorRenderModifier()
{
}

SolidColorRenderModifier::~SolidColorRenderModifier()
{
}

int SolidColorRenderModifier::BeginPass(int UNUSED(pass))
{
	ogl_tex_bind(0, 0);

	return STREAM_POS;
}

bool SolidColorRenderModifier::EndPass(int UNUSED(pass))
{
	return true;
}

void SolidColorRenderModifier::PrepareTexture(int UNUSED(pass), CTexturePtr& UNUSED(texture))
{
}

void SolidColorRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
}


///////////////////////////////////////////////////////////////////////////////////////////////
// ShaderRenderModifier implementation

ShaderRenderModifier::ShaderRenderModifier(const CShaderTechniquePtr& technique) :
	m_Technique(technique)
{
}

ShaderRenderModifier::~ShaderRenderModifier()
{
}

int ShaderRenderModifier::BeginPass(int pass)
{
	m_Technique->BeginPass(pass);

	CShaderProgramPtr shader = m_Technique->GetShader(pass);

	if (GetShadowMap() && shader->HasTexture("shadowTex"))
	{
		shader->BindTexture("shadowTex", GetShadowMap()->GetTexture());
		shader->Uniform("shadowTransform", GetShadowMap()->GetTextureMatrix());

		const float* offsets = GetShadowMap()->GetFilterOffsets();
		shader->Uniform("shadowOffsets1", offsets[0], offsets[1], offsets[2], offsets[3]);
		shader->Uniform("shadowOffsets2", offsets[4], offsets[5], offsets[6], offsets[7]);
	}

	if (GetLightEnv())
	{
		shader->Uniform("ambient", GetLightEnv()->m_UnitsAmbientColor);
		shader->Uniform("sunDir", GetLightEnv()->GetSunDir());
		shader->Uniform("sunColor", GetLightEnv()->m_SunColor);
	}

	if (shader->HasTexture("losTex"))
	{
		CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
		shader->BindTexture("losTex", los.GetTexture());
		// Don't bother sending the whole matrix, we just need two floats (scale and translation)
		shader->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);
	}

	m_BindingInstancingTransform = shader->GetUniformBinding("instancingTransform");
	m_BindingShadingColor = shader->GetUniformBinding("shadingColor");
	m_BindingObjectColor = shader->GetUniformBinding("objectColor");
	m_BindingPlayerColor = shader->GetUniformBinding("playerColor");

	return shader->GetStreamFlags();
}

bool ShaderRenderModifier::EndPass(int pass)
{
	m_Technique->EndPass(pass);

	return (pass >= m_Technique->GetNumPasses()-1);
}

CShaderProgramPtr ShaderRenderModifier::GetShader(int pass)
{
	return m_Technique->GetShader(pass);
}

void ShaderRenderModifier::PrepareTexture(int pass, CTexturePtr& texture)
{
	CShaderProgramPtr shader = m_Technique->GetShader(pass);

	shader->BindTexture("baseTex", texture->GetHandle());
}

void ShaderRenderModifier::PrepareModel(int pass, CModel* model)
{
	CShaderProgramPtr shader = m_Technique->GetShader(pass);

	if (m_BindingInstancingTransform.Active())
		shader->Uniform(m_BindingInstancingTransform, model->GetTransform());

	if (m_BindingShadingColor.Active())
		shader->Uniform(m_BindingShadingColor, model->GetShadingColor());

	if (m_BindingObjectColor.Active())
		shader->Uniform(m_BindingObjectColor, model->GetMaterial().GetObjectColor());

	if (m_BindingPlayerColor.Active())
		shader->Uniform(m_BindingPlayerColor, model->GetMaterial().GetPlayerColor());
}
