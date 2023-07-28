/* Copyright (C) 2021 Wildfire Games.
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

#include "JSInterface_Renderer.h"

#include "graphics/TextureManager.h"
#include "renderer/RenderingOptions.h"
#include "renderer/Renderer.h"
#include "scriptinterface/FunctionWrapper.h"

namespace JSI_Renderer
{
#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME) \
bool Get##NAME##Enabled() \
{ \
	return g_RenderingOptions.Get##NAME(); \
} \
\
void Set##NAME##Enabled(bool enabled) \
{ \
	g_RenderingOptions.Set##NAME(enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING

std::string GetRenderPath()
{
	return RenderPathEnum::ToString(g_RenderingOptions.GetRenderPath());
}

std::string GetRenderDebugMode()
{
	return RenderDebugModeEnum::ToString(g_RenderingOptions.GetRenderDebugMode()).c_str();
}

void SetRenderDebugMode(const std::string& mode)
{
	g_RenderingOptions.SetRenderDebugMode(RenderDebugModeEnum::FromString(mode));
}

bool TextureExists(const std::wstring& filename)
{
	return g_Renderer.GetTextureManager().TextureExists(filename);
}

#define REGISTER_BOOLEAN_SCRIPT_SETTING(NAME) \
ScriptFunction::Register<&Get##NAME##Enabled>(rq, "Renderer_Get" #NAME "Enabled"); \
ScriptFunction::Register<&Set##NAME##Enabled>(rq, "Renderer_Set" #NAME "Enabled");

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&GetRenderPath>(rq, "Renderer_GetRenderPath");
	ScriptFunction::Register<&TextureExists>(rq, "TextureExists");
	ScriptFunction::Register<&GetRenderDebugMode>(rq, "Renderer_GetRenderDebugMode");
	ScriptFunction::Register<&SetRenderDebugMode>(rq, "Renderer_SetRenderDebugMode");
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING
}
