/* Copyright (C) 2019 Wildfire Games.
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

#include "RenderingOptions.h"

#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"
#include "renderer/Renderer.h"

SRenderingOptions g_RenderingOptions;

RenderPath RenderPathEnum::FromString(const CStr8& name)
{
	if (name == "default")
		return DEFAULT;
	if (name == "fixed")
		return FIXED;
	if (name == "shader")
		return SHADER;

	LOGWARNING("Unknown render path %s", name.c_str());
	return DEFAULT;
}

CStr8 RenderPathEnum::ToString(RenderPath path)
{
	switch (path)
	{
	case RenderPath::DEFAULT:
		return "default";
	case RenderPath::FIXED:
		return "fixed";
	case RenderPath::SHADER:
		return "shader";
	}
	return "default"; // Silence warning about reaching end of non-void function.
}

SRenderingOptions::SRenderingOptions()
{
	m_NoVBO = false;
	m_RenderPath = RenderPath::DEFAULT;
	m_Shadows = false;
	m_WaterEffects = false;
	m_WaterFancyEffects = false;
	m_WaterRealDepth = false;
	m_WaterRefraction = false;
	m_WaterReflection = false;
	m_WaterShadows = false;
	m_ShadowAlphaFix = true;
	m_ARBProgramShadow = true;
	m_ShadowPCF = false;
	m_Particles = false;
	m_Silhouettes = false;
	m_PreferGLSL = false;
	m_Fog = false;
	m_ForceAlphaTest = false;
	m_GPUSkinning = false;
	m_SmoothLOS = false;
	m_PostProc = false;
	m_ShowSky = false;
	m_DisplayFrustum = false;
	m_RenderActors = true;
}

void SRenderingOptions::ReadConfig()
{
	// TODO: be more consistent in use of the config system
	CFG_GET_VAL("preferglsl", m_PreferGLSL);
	CFG_GET_VAL("forcealphatest", m_ForceAlphaTest);
	CFG_GET_VAL("gpuskinning", m_GPUSkinning);
	CFG_GET_VAL("smoothlos", m_SmoothLOS);
	CFG_GET_VAL("postproc", m_PostProc);

	CFG_GET_VAL("renderactors", m_RenderActors);
}

void SRenderingOptions::SetShadows(bool value)
{
	m_Shadows = value;
	g_Renderer.MakeShadersDirty();
}

void SRenderingOptions::SetShadowPCF(bool value)
{
	m_ShadowPCF = value;
	g_Renderer.MakeShadersDirty();
}

void SRenderingOptions::SetFog(bool value)
{
	m_Fog = value;
	g_Renderer.MakeShadersDirty();
}

void SRenderingOptions::SetPreferGLSL(bool value)
{
	m_PreferGLSL = value;
	g_Renderer.MakeShadersDirty();
	g_Renderer.RecomputeSystemShaderDefines();
}


void SRenderingOptions::SetRenderPath(RenderPath value)
{
	m_RenderPath = value;
	g_Renderer.SetRenderPath(m_RenderPath);
}
