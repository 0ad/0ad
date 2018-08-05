/* Copyright (C) 2018 Wildfire Games.
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
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"
#include "scriptinterface/ScriptInterface.h"

#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME, SCRIPTNAME) \
bool JSI_Renderer::Get##SCRIPTNAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate)) \
{ \
	return g_Renderer.GetOptionBool(CRenderer::OPT_##NAME); \
} \
\
void JSI_Renderer::Set##SCRIPTNAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool Enabled) \
{ \
	g_Renderer.SetOptionBool(CRenderer::OPT_##NAME, Enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(PARTICLES, Particles);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(PREFERGLSL, PreferGLSL);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WATEREFFECTS, WaterEffects);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WATERFANCYEFFECTS, WaterFancyEffects);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SHADOWPCF, ShadowPCF);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SHADOWS, Shadows);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WATERREALDEPTH, WaterRealDepth);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WATERREFLECTION, WaterReflection);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(WATERREFRACTION, WaterRefraction);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SHADOWSONWATER, WaterShadows);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(FOG, Fog);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SILHOUETTES, Silhouettes);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SHOWSKY, ShowSky);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(SMOOTHLOS, SmoothLOS);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(POSTPROC, Postproc);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DISPLAYFRUSTUM, DisplayFrustum);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING

std::string JSI_Renderer::GetRenderPath(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return CRenderer::GetRenderPathName(g_Renderer.GetRenderPath());
}

void JSI_Renderer::SetRenderPath(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& name)
{
	g_Renderer.SetRenderPath(CRenderer::GetRenderPathByName(name));
}

void JSI_Renderer::RecreateShadowMap(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	g_Renderer.GetShadowMap().RecreateTexture();
}

bool JSI_Renderer::TextureExists(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::wstring& filename)
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
	REGISTER_BOOLEAN_SCRIPT_SETTING(Postproc);
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING
