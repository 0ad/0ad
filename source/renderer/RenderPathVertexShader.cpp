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

#include "precompiled.h"

#include "lib/ogl.h"
#include "lib/res/graphics/ogl_shader.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "renderer/Renderer.h"
#include "renderer/RenderPathVertexShader.h"


void VS_GlobalLight::Init(Handle shader)
{
	m_Ambient = ogl_program_get_uniform_location(shader, "ambient");
	m_SunDir = ogl_program_get_uniform_location(shader, "sunDir");
	m_SunColor = ogl_program_get_uniform_location(shader, "sunColor");
}

void VS_Instancing::Init(Handle shader)
{
	m_Instancing1 = ogl_program_get_attrib_location(shader, "Instancing1");
	m_Instancing2 = ogl_program_get_attrib_location(shader, "Instancing2");
	m_Instancing3 = ogl_program_get_attrib_location(shader, "Instancing3");
}

void VS_PosToUV1::Init(Handle shader)
{
	m_TextureMatrix1 = ogl_program_get_uniform_location(shader, "TextureMatrix1");
	debug_assert(m_TextureMatrix1 >= 0);
	m_TextureMatrix2 = ogl_program_get_uniform_location(shader, "TextureMatrix2");
	debug_assert(m_TextureMatrix2 >= 0);
	m_TextureMatrix3 = ogl_program_get_uniform_location(shader, "TextureMatrix3");
	debug_assert(m_TextureMatrix3 >= 0);
}

RenderPathVertexShader::RenderPathVertexShader()
{
	m_ModelLight = 0;
	m_ModelLightP = 0;
	m_InstancingLight = 0;
	m_Instancing = 0;
}

RenderPathVertexShader::~RenderPathVertexShader()
{
	if (m_ModelLight)
		ogl_program_free(m_ModelLight);
	if (m_ModelLightP)
		ogl_program_free(m_ModelLightP);
	if (m_InstancingLight)
		ogl_program_free(m_InstancingLight);
	if (m_Instancing)
		ogl_program_free(m_Instancing);
}

// Initialize this render path.
// Use delayed initialization so that we can fallback to a different render path
// if anything went wrong and use the destructor to clean things up.
bool RenderPathVertexShader::Init()
{
	if (!g_Renderer.m_Caps.m_VertexShader)
		return false;

	m_ModelLight = ogl_program_load(g_VFS, L"shaders/model_light.xml");
	if (m_ModelLight < 0)
	{
		LOGWARNING(L"Failed to load shaders/model_light.xml: %i\n", (int)m_ModelLight);
		return false;
	}

	m_ModelLightP = ogl_program_load(g_VFS, L"shaders/model_lightp.xml");
	if (m_ModelLightP < 0)
	{
		LOGWARNING(L"Failed to load shaders/model_lightp.xml: %i\n", (int)m_ModelLightP);
		return false;
	}

	m_InstancingLight = ogl_program_load(g_VFS, L"shaders/instancing_light.xml");
	if (m_InstancingLight < 0)
	{
		LOGWARNING(L"Failed to load shaders/instancing_light.xml: %i\n", (int)m_InstancingLight);
		return false;
	}

	m_InstancingLightP = ogl_program_load(g_VFS, L"shaders/instancing_lightp.xml");
	if (m_InstancingLightP < 0)
	{
		LOGWARNING(L"Failed to load shaders/instancing_lightp.xml: %i\n", (int)m_InstancingLightP);
		return false;
	}

	m_Instancing = ogl_program_load(g_VFS, L"shaders/instancing.xml");
	if (m_Instancing < 0)
	{
		LOGWARNING(L"Failed to load shaders/instancing.xml: %i\n", (int)m_Instancing);
		return false;
	}

	m_InstancingP = ogl_program_load(g_VFS, L"shaders/instancingp.xml");
	if (m_InstancingP < 0)
	{
		LOGWARNING(L"Failed to load shaders/instancingp.xml: %i\n", (int)m_InstancingP);
		return false;
	}

	return true;
}


// This is quite the hack, but due to shader reloads,
// the uniform locations might have changed under us.
void RenderPathVertexShader::BeginFrame()
{
	m_ModelLight_Light.Init(m_ModelLight);

	m_ModelLightP_Light.Init(m_ModelLightP);
	m_ModelLightP_PosToUV1.Init(m_ModelLightP);

	m_InstancingLight_Light.Init(m_InstancingLight);
	m_InstancingLight_Instancing.Init(m_InstancingLight);

	m_InstancingLightP_Light.Init(m_InstancingLightP);
	m_InstancingLightP_Instancing.Init(m_InstancingLightP);
	m_InstancingLightP_PosToUV1.Init(m_InstancingLightP);

	m_Instancing_Instancing.Init(m_Instancing);

	m_InstancingP_Instancing.Init(m_InstancingP);
	m_InstancingP_PosToUV1.Init(m_InstancingP);
}
