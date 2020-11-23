/* Copyright (C) 2020 Wildfire Games.
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
#include "renderer/PostprocManager.h"
#include "renderer/ShadowMap.h"

CRenderingOptions g_RenderingOptions;

class CRenderingOptions::ConfigHooks
{
public:
	std::vector<CConfigDB::hook_t>::iterator begin() { return hooks.begin(); }
	std::vector<CConfigDB::hook_t>::iterator end() { return hooks.end(); }
	template<typename T>
	void Setup(CStr8 name, T& variable)
	{
		hooks.emplace_back(g_ConfigDB.RegisterHookAndCall(name, [name, &variable]() { CFG_GET_VAL(name, variable); }));
	}
	void Setup(CStr8 name, std::function<void()> hook)
	{
		hooks.emplace_back(g_ConfigDB.RegisterHookAndCall(name, hook));
	}
	void clear() { hooks.clear(); }
private:
	std::vector<CConfigDB::hook_t> hooks;
};

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

CRenderingOptions::CRenderingOptions() : m_ConfigHooks(new ConfigHooks())
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
	m_DisplayShadowsFrustum = false;
	m_RenderActors = true;
}

CRenderingOptions::~CRenderingOptions()
{
	ClearHooks();
}

void CRenderingOptions::ReadConfigAndSetupHooks()
{
	m_ConfigHooks->Setup("renderpath", [this]() {
		CStr renderPath;
		CFG_GET_VAL("renderpath", renderPath);
		SetRenderPath(RenderPathEnum::FromString(renderPath));
	});

	m_ConfigHooks->Setup("preferglsl", [this]() {
		bool enabled;
		CFG_GET_VAL("preferglsl", enabled);
		SetPreferGLSL(enabled);
	});

	m_ConfigHooks->Setup("shadowquality", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetShadowMap().RecreateTexture();
	});

	m_ConfigHooks->Setup("shadows", [this]() {
		bool enabled;
		CFG_GET_VAL("shadows", enabled);
		SetShadows(enabled);
	});
	m_ConfigHooks->Setup("shadowpcf", [this]() {
		bool enabled;
		CFG_GET_VAL("shadowpcf", enabled);
		SetShadowPCF(enabled);
	});

	m_ConfigHooks->Setup("postproc", m_PostProc);

	m_ConfigHooks->Setup("antialiasing", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetPostprocManager().UpdateAntiAliasingTechnique();
	});

	m_ConfigHooks->Setup("sharpness", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetPostprocManager().UpdateSharpnessFactor();
	});

	m_ConfigHooks->Setup("sharpening", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetPostprocManager().UpdateSharpeningTechnique();
	});

	m_ConfigHooks->Setup("smoothlos", m_SmoothLOS);

	m_ConfigHooks->Setup("watereffects", m_WaterEffects);
	m_ConfigHooks->Setup("waterfancyeffects", m_WaterFancyEffects);
	m_ConfigHooks->Setup("waterrealdepth", m_WaterRealDepth);
	m_ConfigHooks->Setup("waterrefraction", m_WaterRefraction);
	m_ConfigHooks->Setup("waterreflection", m_WaterReflection);
	m_ConfigHooks->Setup("watershadows", m_WaterShadows);

	m_ConfigHooks->Setup("particles", m_Particles);
	m_ConfigHooks->Setup("fog", [this]() {
		bool enabled;
		CFG_GET_VAL("fog", enabled);
		SetFog(enabled);
	});
	m_ConfigHooks->Setup("silhouettes", m_Silhouettes);
	m_ConfigHooks->Setup("showsky", m_ShowSky);

	m_ConfigHooks->Setup("novbo", m_NoVBO);

	m_ConfigHooks->Setup("forcealphatest", m_ForceAlphaTest);
	m_ConfigHooks->Setup("gpuskinning", [this]() {
		bool enabled;
		CFG_GET_VAL("gpuskinning", enabled);
		if (enabled && !m_PreferGLSL)
			LOGWARNING("GPUSkinning has been disabled, because it is not supported with PreferGLSL disabled.");
		else if (enabled)
			m_GPUSkinning = true;
	});

	m_ConfigHooks->Setup("renderactors", m_RenderActors);
}

void CRenderingOptions::ClearHooks()
{
	if (CConfigDB::IsInitialised())
		for (CConfigDB::hook_t& hook : *m_ConfigHooks)
			g_ConfigDB.UnregisterHook(std::move(hook));
	m_ConfigHooks->clear();
}

void CRenderingOptions::SetShadows(bool value)
{
	m_Shadows = value;
	if (CRenderer::IsInitialised())
		g_Renderer.MakeShadersDirty();
}

void CRenderingOptions::SetShadowPCF(bool value)
{
	m_ShadowPCF = value;
	if (CRenderer::IsInitialised())
		g_Renderer.MakeShadersDirty();
}

void CRenderingOptions::SetFog(bool value)
{
	m_Fog = value;
	if (CRenderer::IsInitialised())
		g_Renderer.MakeShadersDirty();
}

void CRenderingOptions::SetPreferGLSL(bool value)
{
	if (m_GPUSkinning && !value)
	{
		LOGWARNING("GPUSkinning have been disabled, because it is not supported with PreferGLSL disabled.");
		m_GPUSkinning = false;
	}
	else if (!m_GPUSkinning && value)
		CFG_GET_VAL("gpuskinning", m_GPUSkinning);

	m_PreferGLSL = value;
	if (!CRenderer::IsInitialised())
		return;
	g_Renderer.MakeShadersDirty();
	g_Renderer.RecomputeSystemShaderDefines();
}


void CRenderingOptions::SetRenderPath(RenderPath value)
{
	m_RenderPath = value;
	if (CRenderer::IsInitialised())
		g_Renderer.SetRenderPath(m_RenderPath);
}
