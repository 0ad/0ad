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
#include "renderer/RenderingOptions.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"

#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME) \
bool Get##NAME##Enabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate)) \
{ \
return g_RenderingOptions.Get##NAME(); \
} \
\
void Set##NAME##Enabled(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), bool enabled) \
{ \
	g_RenderingOptions.Set##NAME(enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING

std::string JSI_Renderer::GetRenderPath(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate))
{
	return RenderPathEnum::ToString(g_RenderingOptions.GetRenderPath());
}

bool JSI_Renderer::TextureExists(ScriptInterface::CmptPrivate* UNUSED(pCmptPrivate), const std::wstring& filename)
{
	return g_Renderer.GetTextureManager().TextureExists(filename);
}

#define REGISTER_BOOLEAN_SCRIPT_SETTING(NAME) \
scriptInterface.RegisterFunction<bool, &Get##NAME##Enabled>("Renderer_Get" #NAME "Enabled"); \
scriptInterface.RegisterFunction<void, bool, &Set##NAME##Enabled>("Renderer_Set" #NAME "Enabled");

void JSI_Renderer::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<std::string, &JSI_Renderer::GetRenderPath>("Renderer_GetRenderPath");
	scriptInterface.RegisterFunction<bool, std::wstring, &JSI_Renderer::TextureExists>("TextureExists");
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
	REGISTER_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING
