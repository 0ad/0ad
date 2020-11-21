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
	void insert(CConfigDB::hook_t&& hook) { return hooks.emplace_back(std::move(hook)); }
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
	// This is currently irrelevant since CConfigDB is deleted before CRenderingOptions
	// (as only the latter is a static variable), but the check is a good idea regardless.
	if (!CConfigDB::IsInitialised())
		return;
	for (CConfigDB::hook_t& hook : *m_ConfigHooks)
		g_ConfigDB.UnregisterHook(std::move(hook));
}

template<typename T>
void CRenderingOptions::SetupConfig(CStr8 name, T& variable)
{
	m_ConfigHooks->insert(g_ConfigDB.RegisterHookAndCall(name, [name, &variable]() { CFG_GET_VAL(name, variable); }));
}

void CRenderingOptions::SetupConfig(CStr8 name, std::function<void()> hook)
{
	m_ConfigHooks->insert(g_ConfigDB.RegisterHookAndCall(name, hook));
}

void CRenderingOptions::ReadConfig()
{
	SetupConfig("preferglsl", [this]() {
		bool enabled;
		CFG_GET_VAL("preferglsl", enabled);
		SetPreferGLSL(enabled);
	});

	SetupConfig("shadowquality", []() {
		g_Renderer.GetShadowMap().RecreateTexture();
	});

	SetupConfig("shadows", [this]() {
		bool enabled;
		CFG_GET_VAL("shadows", enabled);
		SetShadows(enabled);
	});
	SetupConfig("shadowpcf", [this]() {
		bool enabled;
		CFG_GET_VAL("shadowpcf", enabled);
		SetShadowPCF(enabled);
	});

	SetupConfig("antialiasing", []() {
		g_Renderer.GetPostprocManager().UpdateAntiAliasingTechnique();
	});

	SetupConfig("sharpness", []() {
		g_Renderer.GetPostprocManager().UpdateSharpnessFactor();
	});

	SetupConfig("sharpening", []() {
		g_Renderer.GetPostprocManager().UpdateSharpeningTechnique();
	});

	SetupConfig("postproc", m_PostProc);
	SetupConfig("smoothlos", m_SmoothLOS);

	SetupConfig("renderpath", [this]() {
		CStr renderPath;
		CFG_GET_VAL("renderpath", renderPath);
		SetRenderPath(RenderPathEnum::FromString(renderPath));
	});

	SetupConfig("watereffects", m_WaterEffects);
	SetupConfig("waterfancyeffects", m_WaterFancyEffects);
	SetupConfig("waterrealdepth", m_WaterRealDepth);
	SetupConfig("waterrefraction", m_WaterRefraction);
	SetupConfig("waterreflection", m_WaterReflection);
	SetupConfig("watershadows", m_WaterShadows);

	SetupConfig("particles", m_Particles);
	SetupConfig("fog", [this]() {
		bool enabled;
		CFG_GET_VAL("fog", enabled);
		SetFog(enabled);
	});
	SetupConfig("silhouettes", m_Silhouettes);
	SetupConfig("showsky", m_ShowSky);

	SetupConfig("novbo", m_NoVBO);

	SetupConfig("forcealphatest", m_ForceAlphaTest);
	SetupConfig("gpuskinning", [this]() {
		bool enabled;
		CFG_GET_VAL("gpuskinning", enabled);
		if (enabled && !m_PreferGLSL)
			LOGWARNING("GPUSkinning has been disabled, because it is not supported with PreferGLSL disabled.");
		else if (enabled)
			m_GPUSkinning = true;
	});

	SetupConfig("renderactors", m_RenderActors);
}

void CRenderingOptions::SetShadows(bool value)
{
	m_Shadows = value;
	g_Renderer.MakeShadersDirty();
}

void CRenderingOptions::SetShadowPCF(bool value)
{
	m_ShadowPCF = value;
	g_Renderer.MakeShadersDirty();
}

void CRenderingOptions::SetFog(bool value)
{
	m_Fog = value;
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
	g_Renderer.MakeShadersDirty();
	g_Renderer.RecomputeSystemShaderDefines();
}


void CRenderingOptions::SetRenderPath(RenderPath value)
{
	m_RenderPath = value;
	g_Renderer.SetRenderPath(m_RenderPath);
}
