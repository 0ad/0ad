/* Copyright (C) 2009 Wildfire Games.
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

#include "ps/CLogger.h"

#include "graphics/LightEnv.h"
#include "graphics/Model.h"

#include "renderer/RenderModifiers.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"

#define LOG_CATEGORY "graphics"



///////////////////////////////////////////////////////////////////////////////////////////////
// RenderModifier implementation

const CMatrix3D* RenderModifier::GetTexGenMatrix(int UNUSED(pass))
{
	debug_warn("GetTexGenMatrix not implemented by a derived RenderModifier");
	return 0;
}

void RenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
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
	debug_assert(pass == 0);

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

void PlainRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}


///////////////////////////////////////////////////////////////////////////////////////////////
// PlainLitRenderModifier implementation

PlainLitRenderModifier::PlainLitRenderModifier()
{
}

PlainLitRenderModifier::~PlainLitRenderModifier()
{
}

int PlainLitRenderModifier::BeginPass(int pass)
{
	debug_assert(pass == 0);
	debug_assert(GetShadowMap() && GetShadowMap()->GetUseDepthTexture());

	// Ambient + Diffuse * Shadow
	pglActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE1);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	pglActiveTextureARB(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetShadowMap()->GetTexture());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &GetLightEnv()->m_UnitsAmbientColor.X);

	// Incoming color is ambient + diffuse light
	pglActiveTextureARB(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, GetShadowMap()->GetTexture());
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	pglActiveTextureARB(GL_TEXTURE0);

	return STREAM_POS|STREAM_COLOR|STREAM_UV0|STREAM_TEXGENTOUV1;
}

const CMatrix3D* PlainLitRenderModifier::GetTexGenMatrix(int UNUSED(pass))
{
	return &GetShadowMap()->GetTextureMatrix();
}

bool PlainLitRenderModifier::EndPass(int UNUSED(pass))
{
	g_Renderer.BindTexture(1, 0);
	g_Renderer.BindTexture(2, 0);
	pglActiveTextureARB(GL_TEXTURE0);
	pglClientActiveTextureARB(GL_TEXTURE0);

	return true;
}

void PlainLitRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
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
	debug_assert(pass == 0);

	// first switch on wireframe
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// setup some renderstate ..
	glDepthMask(0);
	g_Renderer.SetTexture(0,0);
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


void WireframeRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* UNUSED(texture))
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
	g_Renderer.SetTexture(0,0);

	return STREAM_POS;
}

bool SolidColorRenderModifier::EndPass(int UNUSED(pass))
{
	return true;
}

void SolidColorRenderModifier::PrepareTexture(int UNUSED(pass), CTexture* UNUSED(texture))
{
}

void SolidColorRenderModifier::PrepareModel(int UNUSED(pass), CModel* UNUSED(model))
{
}
