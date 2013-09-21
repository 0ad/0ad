/* Copyright (C) 2013 Wildfire Games.
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

class ScriptInterface;

#define DECLARE_BOOLEAN_SCRIPT_SETTING(NAME) \
	bool Get##NAME##Enabled(void* cbdata); \
	void Set##NAME##Enabled(void* cbdata, bool Enabled);

namespace JSI_Renderer
{
	std::string GetRenderPath(void* cbdata);
	void SetRenderPath(void* cbdata, std::string name);

	DECLARE_BOOLEAN_SCRIPT_SETTING(Shadows);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ShadowPCF);
	DECLARE_BOOLEAN_SCRIPT_SETTING(Particles);
	DECLARE_BOOLEAN_SCRIPT_SETTING(PreferGLSL);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterNormal);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterRealDepth);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterReflection);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterRefraction);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterFoam);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterCoastalWaves);
	DECLARE_BOOLEAN_SCRIPT_SETTING(WaterShadow);
	DECLARE_BOOLEAN_SCRIPT_SETTING(Silhouettes);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ShowSky);

	void RegisterScriptFunctions(ScriptInterface& scriptInterface);
}

#undef DECLARE_BOOLEAN_SCRIPT_SETTING

#endif
