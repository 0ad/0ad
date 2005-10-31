/**
 * =========================================================================
 * File        : RenderModifiers.cpp
 * Project     : Pyrogenesis
 * Description : Implementation of common RenderModifiers
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "ogl.h"
#include "Vector3D.h"
#include "Vector4D.h"

#include "ps/CLogger.h"

#include "graphics/Color.h"
#include "graphics/Model.h"

#include "renderer/RenderModifiers.h"
#include "renderer/Renderer.h"

#define LOG_CATEGORY "graphics"



///////////////////////////////////////////////////////////////////////////////////////////////
// RenderModifier implementation

void RenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
{
}


///////////////////////////////////////////////////////////////////////////////////////////////
// PlainRenderModifier implementation

PlainRenderModifier::PlainRenderModifier()
{
}

PlainRenderModifier::~PlainRenderModifier()
{
}

u32 PlainRenderModifier::BeginPass(uint pass)
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

bool PlainRenderModifier::EndPass(uint UNUSED(pass))
{
	// We didn't modify blend state or higher texenvs, so we don't have
	// to reset OpenGL state here.
	
	return true;
}

void PlainRenderModifier::PrepareTexture(uint UNUSED(pass), CTexture* texture)
{
	g_Renderer.SetTexture(0, texture);
}

void PlainRenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
{
}


///////////////////////////////////////////////////////////////////////////////////////////////
// WireframeRenderModifier implementation


WireframeRenderModifier::WireframeRenderModifier()
{
}

WireframeRenderModifier::~WireframeRenderModifier()
{
}

u32 WireframeRenderModifier::BeginPass(uint pass)
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


bool WireframeRenderModifier::EndPass(uint UNUSED(pass))
{
	// .. restore the renderstates
	glDisable(GL_BLEND);
	glDepthMask(1);

	// restore fill mode, and we're done
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

	return true;
}


void WireframeRenderModifier::PrepareTexture(uint UNUSED(pass), CTexture* UNUSED(texture))
{
}


void WireframeRenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
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

u32 SolidColorRenderModifier::BeginPass(uint UNUSED(pass))
{
	g_Renderer.SetTexture(0,0);
	
	return STREAM_POS;
}

bool SolidColorRenderModifier::EndPass(uint UNUSED(pass))
{
	return true;
}

void SolidColorRenderModifier::PrepareTexture(uint UNUSED(pass), CTexture* UNUSED(texture))
{
}

void SolidColorRenderModifier::PrepareModel(uint UNUSED(pass), CModel* UNUSED(model))
{
}
