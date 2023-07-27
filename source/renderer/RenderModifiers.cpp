/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "renderer/RenderModifiers.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Model.h"
#include "graphics/TextureManager.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "maths/Matrix3D.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/ShadowMap.h"

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
	: m_ShadingColor(1.0f, 1.0f, 1.0f, 1.0f), m_PlayerColor(1.0f, 1.0f, 1.0f, 1.0f)
{
}

void ShaderRenderModifier::BeginPass(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IShaderProgram* shader)
{
	const CMatrix3D transform =
		g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection();
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_transform), transform.AsFloatArray());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_cameraPos),
		g_Renderer.GetSceneRenderer().GetViewCamera().GetOrientation().GetTranslation().AsFloatArray());

	if (GetShadowMap())
		GetShadowMap()->BindTo(deviceCommandContext, shader);

	if (GetLightEnv())
	{
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_ambient),
			GetLightEnv()->m_AmbientColor.AsFloatArray());
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_sunDir),
			GetLightEnv()->GetSunDir().AsFloatArray());
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_sunColor),
			GetLightEnv()->m_SunColor.AsFloatArray());

		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_fogColor),
			GetLightEnv()->m_FogColor.AsFloatArray());
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_fogParams),
			GetLightEnv()->m_FogFactor, GetLightEnv()->m_FogMax);
	}

	if (shader->GetBindingSlot(str_losTex) >= 0)
	{
		CLOSTexture& los = g_Renderer.GetSceneRenderer().GetScene().GetLOSTexture();
		deviceCommandContext->SetTexture(
			shader->GetBindingSlot(str_losTex), los.GetTextureSmooth());
		// Don't bother sending the whole matrix, we just need two floats (scale and translation)
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_losTransform),
			los.GetTextureMatrix()[0], los.GetTextureMatrix()[12]);
	}

	m_BindingInstancingTransform = shader->GetBindingSlot(str_instancingTransform);
	m_BindingShadingColor = shader->GetBindingSlot(str_shadingColor);
	m_BindingPlayerColor = shader->GetBindingSlot(str_playerColor);

	if (m_BindingShadingColor >= 0)
	{
		m_ShadingColor = CColor(1.0f, 1.0f, 1.0f, 1.0f);
		deviceCommandContext->SetUniform(
			m_BindingShadingColor, m_ShadingColor.AsFloatArray());
	}

	if (m_BindingPlayerColor >= 0)
	{
		m_PlayerColor = g_Game->GetPlayerColor(0);
		deviceCommandContext->SetUniform(
			m_BindingPlayerColor, m_PlayerColor.AsFloatArray());
	}
}

void ShaderRenderModifier::PrepareModel(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CModel* model)
{
	if (m_BindingInstancingTransform >= 0)
	{
		deviceCommandContext->SetUniform(
			m_BindingInstancingTransform, model->GetTransform().AsFloatArray());
	}

	if (m_BindingShadingColor >= 0 && m_ShadingColor != model->GetShadingColor())
	{
		m_ShadingColor = model->GetShadingColor();
		deviceCommandContext->SetUniform(
			m_BindingShadingColor, m_ShadingColor.AsFloatArray());
	}

	if (m_BindingPlayerColor >= 0)
	{
		const CColor& playerColor = g_Game->GetPlayerColor(model->GetPlayerID());
		if (m_PlayerColor != playerColor)
		{
			m_PlayerColor = playerColor;
			deviceCommandContext->SetUniform(
			m_BindingPlayerColor, m_PlayerColor.AsFloatArray());
		}
	}
}
