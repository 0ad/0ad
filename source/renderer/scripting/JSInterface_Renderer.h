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

#ifndef INCLUDED_JSINTERFACE_RENDERER
#define INCLUDED_JSINTERFACE_RENDERER

#include "scriptinterface/ScriptInterface.h"

#define DECLARE_BOOLEAN_SCRIPT_SETTING(NAME) \
	bool Get##NAME##Enabled(ScriptInterface::CmptPrivate* pCmptPrivate); \
	void Set##NAME##Enabled(ScriptInterface::CmptPrivate* pCmptPrivate, bool Enabled);

namespace JSI_Renderer
{
	std::string GetRenderPath(ScriptInterface::CmptPrivate* pCmptPrivate);
	void SetRenderPath(ScriptInterface::CmptPrivate* pCmptPrivate, const std::string& name);
	void UpdateAntiAliasingTechnique(ScriptInterface::CmptPrivate* pCmptPrivate);
	void UpdateSharpeningTechnique(ScriptInterface::CmptPrivate* pCmptPrivate);
	void UpdateSharpnessFactor(ScriptInterface::CmptPrivate* pCmptPrivate);
	void RecreateShadowMap(ScriptInterface::CmptPrivate* pCmptPrivate);
	bool TextureExists(ScriptInterface::CmptPrivate* pCmptPrivate, const std::wstring& filename);

	DECLARE_BOOLEAN_SCRIPT_SETTING(Shadows);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ShadowPCF);
	DECLARE_BOOLEAN_SCRIPT_SETTING(Particles);
	DECLARE_BOOLEAN_SCRIPT_SETTING(PreferGLSL);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterEffects);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterFancyEffects);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterRealDepth);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterReflection);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterRefraction);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterShadows);
	DECLARE_BOOLEAN_SCRIPT_SETTING(Fog);
	DECLARE_BOOLEAN_SCRIPT_SETTING(Silhouettes);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ShowSky);
	DECLARE_BOOLEAN_SCRIPT_SETTING(SmoothLOS);
	DECLARE_BOOLEAN_SCRIPT_SETTING(PostProc);
	DECLARE_BOOLEAN_SCRIPT_SETTING(DisplayFrustum);
	DECLARE_BOOLEAN_SCRIPT_SETTING(DisplayShadowsFrustum);

	void RegisterScriptFunctions(const ScriptInterface& scriptInterface);
}

#undef DECLARE_BOOLEAN_SCRIPT_SETTING

#endif // INCLUDED_JSINTERFACE_RENDERER
