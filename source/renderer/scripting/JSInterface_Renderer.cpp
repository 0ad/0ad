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

#include "JSInterface_Renderer.h"

#include "graphics/TextureManager.h"
#include "renderer/PostprocManager.h"
#include "renderer/RenderingOptions.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"
#include "scriptinterface/ScriptInterface.h"

#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME) \
bool JSI_Renderer::Get##NAME##Enabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate)) \
{ \
return g_RenderingOptions.Get##NAME(); \
} \
\
void JSI_Renderer::Set##NAME##Enabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool enabled) \
{ \
	g_RenderingOptions.Set##NAME(enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Shadows);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(ShadowPCF);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Particles);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(PreferGLSL);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterEffects);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterFancyEffects);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterRealDepth);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterReflection);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterRefraction);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WaterShadows);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Fog);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Silhouettes);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(ShowSky);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SmoothLOS);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(PostProc);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING

std::string JSI_Renderer::GetRenderPath(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return RenderPathEnum::ToString(g_RenderingOptions.GetRenderPath());
}

void JSI_Renderer::SetRenderPath(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::string& name)
{
	g_RenderingOptions.SetRenderPath(RenderPathEnum::FromString(name));
}

void JSI_Renderer::UpdateAntiAliasingTechnique(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	g_Renderer.GetPostprocManager().UpdateAntiAliasingTechnique();
}

void JSI_Renderer::UpdateSharpeningTechnique(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	g_Renderer.GetPostprocManager().UpdateSharpeningTechnique();
}

void JSI_Renderer::UpdateSharpnessFactor(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	g_Renderer.GetPostprocManager().UpdateSharpnessFactor();
}

void JSI_Renderer::RecreateShadowMap(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	g_Renderer.GetShadowMap().RecreateTexture();
}

bool JSI_Renderer::TextureExists(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename)
{
	return g_Renderer.GetTextureManager().TextureExists(filename);
}

#define REGISTER_BOOLEAN_SCRIPT_SETTING(NAME) \
scriptInterface.RegisterFunction<bool, &JSI_Renderer::Get##NAME##Enabled>("Renderer_Get" #NAME "Enabled"); \
scriptInterface.RegisterFunction<void, bool, &JSI_Renderer::Set##NAME##Enabled>("Renderer_Set" #NAME "Enabled");

void JSI_Renderer::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<std::string, &JSI_Renderer::GetRenderPath>("Renderer_GetRenderPath");
	scriptInterface.RegisterFunction<void, std::string, &JSI_Renderer::SetRenderPath>("Renderer_SetRenderPath");
	scriptInterface.RegisterFunction<void, &JSI_Renderer::RecreateShadowMap>("Renderer_RecreateShadowMap");
	scriptInterface.RegisterFunction<void, &JSI_Renderer::UpdateAntiAliasingTechnique>("Renderer_UpdateAntiAliasingTechnique");
	scriptInterface.RegisterFunction<void, &JSI_Renderer::UpdateSharpeningTechnique>("Renderer_UpdateSharpeningTechnique");
	scriptInterface.RegisterFunction<void, &JSI_Renderer::UpdateSharpnessFactor>("Renderer_UpdateSharpnessFactor");
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_Renderer::TextureExists>("TextureExists");
	REGISTER_BOOLEAN_SCRIPT_SETTING(Shadows);
	REGISTER_BOOLEAN_SCRIPT_SETTING(ShadowPCF);
	REGISTER_BOOLEAN_SCRIPT_SETTING(Particles);
	REGISTER_BOOLEAN_SCRIPT_SETTING(PreferGLSL);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterEffects);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterFancyEffects);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterRealDepth);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterReflection);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterRefraction);
	REGISTER_BOOLEAN_SCRIPT_SETTING(WaterShadows);
	REGISTER_BOOLEAN_SCRIPT_SETTING(Fog);
	REGISTER_BOOLEAN_SCRIPT_SETTING(Silhouettes);
	REGISTER_BOOLEAN_SCRIPT_SETTING(ShowSky);
	REGISTER_BOOLEAN_SCRIPT_SETTING(SmoothLOS);
	REGISTER_BOOLEAN_SCRIPT_SETTING(PostProc);
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING
