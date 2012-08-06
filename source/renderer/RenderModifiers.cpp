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
// ShaderRenderModifier implementation

ShaderRenderModifier::ShaderRenderModifier()
{
}

void ShaderRenderModifier::BeginPass(const CShaderProgramPtr& shader)
{
	shader->Uniform("transform", g_Renderer.GetViewCamera().GetViewProjection());
	shader->Uniform("cameraPos", g_Renderer.GetViewCamera().GetOrientation().GetTranslation());

	if (GetShadowMap() && shader->GetTextureBinding("shadowTex").Active())
	{
		shader->BindTexture("shadowTex", GetShadowMap()->GetTexture());
		shader->Uniform("shadowTransform", GetShadowMap()->GetTextureMatrix());
		int width = GetShadowMap()->GetWidth();
		int height = GetShadowMap()->GetHeight();
		shader->Uniform("shadowScale", width, height, 1.0f / width, 1.0f / height); 
	}

	if (GetLightEnv())
	{
		shader->Uniform("ambient", GetLightEnv()->m_UnitsAmbientColor);
		shader->Uniform("sunDir", GetLightEnv()->GetSunDir());
		shader->Uniform("sunColor", GetLightEnv()->m_SunColor);
	}

	if (shader->GetTextureBinding("losTex").Active())
	{
		CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
		shader->BindTexture("losTex", los.GetTextureSmooth());
		// Don't bother sending the whole matrix, we just need two floats (scale and translation)
		shader->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);
	}

	m_BindingInstancingTransform = shader->GetUniformBinding("instancingTransform");
	m_BindingShadingColor = shader->GetUniformBinding("shadingColor");
	m_BindingPlayerColor = shader->GetUniformBinding("playerColor");
}

void ShaderRenderModifier::PrepareModel(const CShaderProgramPtr& shader, CModel* model)
{
	if (m_BindingInstancingTransform.Active())
		shader->Uniform(m_BindingInstancingTransform, model->GetTransform());

	if (m_BindingShadingColor.Active())
		shader->Uniform(m_BindingShadingColor, model->GetShadingColor());

	if (m_BindingPlayerColor.Active())
		shader->Uniform(m_BindingPlayerColor, g_Game->GetPlayerColour(model->GetPlayerID()));
}
