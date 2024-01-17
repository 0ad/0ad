/* Copyright (C) 2024 Wildfire Games.
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

#include "graphics/TextureManager.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStr.h"
#include "ps/CStrInternStatic.h"
#include "ps/VideoMode.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"
#include "renderer/PostprocManager.h"
#include "renderer/SceneRenderer.h"
#include "renderer/ShadowMap.h"
#include "renderer/WaterManager.h"

CRenderingOptions g_RenderingOptions;

class CRenderingOptions::ConfigHooks
{
public:
	std::vector<CConfigDBHook>::iterator begin() { return hooks.begin(); }
	std::vector<CConfigDBHook>::iterator end() { return hooks.end(); }
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
	std::vector<CConfigDBHook> hooks;
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

RenderDebugMode RenderDebugModeEnum::FromString(const CStr8& name)
{
	if (name == str_RENDER_DEBUG_MODE_NONE.c_str())
		return RenderDebugMode::NONE;
	if (name == str_RENDER_DEBUG_MODE_AO.c_str())
		return RenderDebugMode::AO;
	if (name == str_RENDER_DEBUG_MODE_ALPHA.c_str())
		return RenderDebugMode::ALPHA;
	if (name == str_RENDER_DEBUG_MODE_CUSTOM.c_str())
		return RenderDebugMode::CUSTOM;

	LOGWARNING("Unknown render debug mode %s", name.c_str());
	return RenderDebugMode::NONE;
}

CStrIntern RenderDebugModeEnum::ToString(RenderDebugMode mode)
{
	switch (mode)
	{
	case RenderDebugMode::AO:
		return str_RENDER_DEBUG_MODE_AO;
	case RenderDebugMode::ALPHA:
		return str_RENDER_DEBUG_MODE_ALPHA;
	case RenderDebugMode::CUSTOM:
		return str_RENDER_DEBUG_MODE_CUSTOM;
	default:
		break;
	}
	return str_RENDER_DEBUG_MODE_NONE;
}

CRenderingOptions::CRenderingOptions() : m_ConfigHooks(new ConfigHooks())
{
	m_RenderPath = RenderPath::DEFAULT;
	m_Shadows = false;
	m_WaterEffects = false;
	m_WaterFancyEffects = false;
	m_WaterRealDepth = false;
	m_WaterRefraction = false;
	m_WaterReflection = false;
	m_ShadowAlphaFix = false;
	m_ShadowPCF = false;
	m_Particles = false;
	m_Silhouettes = false;
	m_Fog = false;
	m_GPUSkinning = false;
	m_SmoothLOS = false;
	m_PostProc = false;
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

	m_ConfigHooks->Setup("shadowquality", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetShadowMap().RecreateTexture();
	});
	m_ConfigHooks->Setup("shadowscascadecount", []() {
		if (CRenderer::IsInitialised())
		{
			g_Renderer.GetSceneRenderer().GetShadowMap().RecreateTexture();
			g_Renderer.MakeShadersDirty();
		}
	});
	m_ConfigHooks->Setup("shadowscovermap", []() {
		if (CRenderer::IsInitialised())
		{
			g_Renderer.GetSceneRenderer().GetShadowMap().RecreateTexture();
			g_Renderer.MakeShadersDirty();
		}
	});
	m_ConfigHooks->Setup("shadowscutoffdistance", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetShadowMap().RecreateTexture();
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

	m_ConfigHooks->Setup("renderer.scale", []()
		{
			if (CRenderer::IsInitialised())
				g_Renderer.GetPostprocManager().Resize();
		});

	m_ConfigHooks->Setup("renderer.upscale.technique", []()
		{
			CStr upscaleName;
			CFG_GET_VAL("renderer.upscale.technique", upscaleName);
			if (CRenderer::IsInitialised())
				g_Renderer.GetPostprocManager().SetUpscaleTechnique(upscaleName);
		});

	m_ConfigHooks->Setup("smoothlos", m_SmoothLOS);

	m_ConfigHooks->Setup("watereffects", [this]() {
		bool enabled;
		CFG_GET_VAL("watereffects", enabled);
		SetWaterEffects(enabled);
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetWaterManager().RecreateOrLoadTexturesIfNeeded();
	});
	m_ConfigHooks->Setup("waterfancyeffects", [this]() {
		bool enabled;
		CFG_GET_VAL("waterfancyeffects", enabled);
		SetWaterFancyEffects(enabled);
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetWaterManager().RecreateOrLoadTexturesIfNeeded();
	});
	m_ConfigHooks->Setup("waterrealdepth", m_WaterRealDepth);
	m_ConfigHooks->Setup("waterrefraction", [this]() {
		bool enabled;
		CFG_GET_VAL("waterrefraction", enabled);
		SetWaterRefraction(enabled);
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetWaterManager().RecreateOrLoadTexturesIfNeeded();
	});
	m_ConfigHooks->Setup("waterreflection", [this]() {
		bool enabled;
		CFG_GET_VAL("waterreflection", enabled);
		SetWaterReflection(enabled);
		if (CRenderer::IsInitialised())
			g_Renderer.GetSceneRenderer().GetWaterManager().RecreateOrLoadTexturesIfNeeded();
	});

	m_ConfigHooks->Setup("particles", m_Particles);
	m_ConfigHooks->Setup("fog", [this]() {
		bool enabled;
		CFG_GET_VAL("fog", enabled);
		SetFog(enabled);
	});
	m_ConfigHooks->Setup("silhouettes", m_Silhouettes);

	m_ConfigHooks->Setup("gpuskinning", [this]() {
		bool enabled;
		CFG_GET_VAL("gpuskinning", enabled);
		if (enabled)
		{
			if (g_VideoMode.GetBackendDevice()->GetBackend() == Renderer::Backend::Backend::GL_ARB)
				LOGWARNING("GPUSkinning has been disabled, because it is not supported with ARB shaders.");
			else if (g_VideoMode.GetBackendDevice()->GetBackend() == Renderer::Backend::Backend::VULKAN)
				LOGWARNING("GPUSkinning has been disabled, because it is not supported for Vulkan backend yet.");
			else
				m_GPUSkinning = true;
		}
	});

	m_ConfigHooks->Setup("renderactors", m_RenderActors);

	m_ConfigHooks->Setup("textures.quality", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetTextureManager().OnQualityChanged();
	});
	m_ConfigHooks->Setup("textures.maxanisotropy", []() {
		if (CRenderer::IsInitialised())
			g_Renderer.GetTextureManager().OnQualityChanged();
	});
}

void CRenderingOptions::ClearHooks()
{
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

void CRenderingOptions::SetRenderPath(RenderPath value)
{
	m_RenderPath = value;
	if (CRenderer::IsInitialised())
		g_Renderer.SetRenderPath(m_RenderPath);
}

void CRenderingOptions::SetRenderDebugMode(RenderDebugMode value)
{
	m_RenderDebugMode = value;
	if (CRenderer::IsInitialised())
		g_Renderer.MakeShadersDirty();
}
