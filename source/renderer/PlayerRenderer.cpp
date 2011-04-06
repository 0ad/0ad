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
 * Implementation of player colour RenderModifiers.
 */

#include "precompiled.h"

#include "renderer/Renderer.h"
#include "renderer/PlayerRenderer.h"
#include "renderer/ShadowMap.h"

#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/TextureManager.h"



///////////////////////////////////////////////////////////////////////////////////////////////////
// FastPlayerColorRender

FastPlayerColorRender::FastPlayerColorRender()
{
	debug_assert(ogl_max_tex_units >= 3);
}

FastPlayerColorRender::~FastPlayerColorRender()
{
}

bool FastPlayerColorRender::IsAvailable()
{
	return (ogl_max_tex_units >= 3);
}

int FastPlayerColorRender::BeginPass(int pass)
{
	debug_assert(pass == 0);

	// Fast player color uses a single pass with three texture environments
	// Note: This uses ARB_texture_env_crossbar (which is checked in GameSetup)
	//
	// We calculate: Result = Color*Texture*(PlayerColor*(1-Texture.a) + 1.0*Texture.a)
	// Algebra gives us:
	// Result = (1 - ((1 - PlayerColor) * (1 - Texture.a)))*Texture*Color

	// TexEnv #0
	pglActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_ONE_MINUS_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);

	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// TexEnv #1
	pglActiveTextureARB(GL_TEXTURE0+1);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_ONE_MINUS_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// TexEnv #2
	pglActiveTextureARB(GL_TEXTURE0+2);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Don't care about alpha; set it to something harmless
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	pglActiveTextureARB(GL_TEXTURE0);

	return STREAM_POS|STREAM_COLOR|STREAM_UV0;
}


bool FastPlayerColorRender::EndPass(int UNUSED(pass))
{
	// Restore state
	pglActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	pglActiveTextureARB(GL_TEXTURE2);
	glDisable(GL_TEXTURE_2D);
	pglActiveTextureARB(GL_TEXTURE0);

	return true;
}

void FastPlayerColorRender::PrepareTexture(int UNUSED(pass), CTexturePtr& texture)
{
	texture->Bind(2);
	texture->Bind(1);
	texture->Bind(0);
}

void FastPlayerColorRender::PrepareModel(int UNUSED(pass), CModel* model)
{
	// Get the player color
	SMaterialColor colour = model->GetMaterial().GetObjectColor();
	float* color = &colour.r; // because it's stored RGBA

	// Set the texture environment color the player color
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// SlowPlayerColorRender

SlowPlayerColorRender::SlowPlayerColorRender()
{
}

SlowPlayerColorRender::~SlowPlayerColorRender()
{
}

int SlowPlayerColorRender::BeginPass(int pass)
{
	// We calculate: Result = (Color*Texture)*Texture.a + (Color*Texture*PlayerColor)*(1-Texture.a)
	// Modulation is done via texture environments, the final interpolation is done via blending

	if (pass == 0)
	{
		// TexEnv #0
		pglActiveTextureARB(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

		// Don't care about alpha; set it to something harmless
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		// Render it!
		return STREAM_POS|STREAM_COLOR|STREAM_UV0;
	}
	else
	{
		// TexEnv #0
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

		// Alpha = Opacity of non-player colored layer
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

		// TexEnv #1
		pglActiveTextureARB(GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

		// Pass alpha unchanged
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
		pglActiveTextureARB(GL_TEXTURE0);

		// Setup blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_LESS, 1.0);
		glDepthMask(0);

		// Render it!
		return STREAM_POS|STREAM_COLOR|STREAM_UV0;
	}
}

bool SlowPlayerColorRender::EndPass(int pass)
{
	if (pass == 0)
		return false; // need two passes

	// Restore state
	pglActiveTextureARB(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	pglActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(1);

	return true;
}

void SlowPlayerColorRender::PrepareTexture(int pass, CTexturePtr& texture)
{
	if (pass == 1)
		texture->Bind(1);
	texture->Bind(0);
}


void SlowPlayerColorRender::PrepareModel(int pass, CModel* model)
{
	if (pass == 1)
	{
		// Get the player color
		SMaterialColor colour = model->GetMaterial().GetObjectColor();
		float* color = &colour.r; // because it's stored RGBA

		// Set the texture environment color the player color
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
// SolidPlayerColorRender

SolidPlayerColorRender::SolidPlayerColorRender()
{
}

SolidPlayerColorRender::~SolidPlayerColorRender()
{
}

int SolidPlayerColorRender::BeginPass(int UNUSED(pass))
{
	ogl_tex_bind(0, 0);

	return STREAM_POS;
}

bool SolidPlayerColorRender::EndPass(int UNUSED(pass))
{
	return true;
}

void SolidPlayerColorRender::PrepareTexture(int UNUSED(pass), CTexturePtr& UNUSED(texture))
{
}

void SolidPlayerColorRender::PrepareModel(int UNUSED(pass), CModel* model)
{
	// Get the player color
	SMaterialColor colour = model->GetMaterial().GetPlayerColor();

	// Send the player color
	glColor3f(colour.r, colour.g, colour.b);
}
