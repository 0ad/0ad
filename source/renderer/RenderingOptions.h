/* Copyright (C) 2022 Wildfire Games.
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

/**
 * Keeps track of the settings used for rendering.
 * Ideally this header file should remain very quick to parse,
 * so avoid including other headers here unless absolutely necessary.
 *
 * Lifetime concerns: g_RenderingOptions always exists, but hooks are tied to the configDB's lifetime
 * an the renderer may or may not actually exist.
 */

#ifndef INCLUDED_RENDERINGOPTIONS
#define INCLUDED_RENDERINGOPTIONS

#include "ps/CStr.h"
#include "ps/CStrIntern.h"

class CRenderer;

enum RenderPath
{
	// If no rendering path is configured explicitly, the renderer
	// will choose the path when Open() is called.
	DEFAULT,

	// Classic fixed function.
	FIXED,

	// Use new ARB/GLSL system
	SHADER
};

struct RenderPathEnum
{
	static RenderPath FromString(const CStr8& name);
	static CStr8 ToString(RenderPath);
};

enum class RenderDebugMode
{
	NONE,
	AO,
	ALPHA,
	CUSTOM
};

struct RenderDebugModeEnum
{
	static RenderDebugMode FromString(const CStr8& name);
	static CStrIntern ToString(RenderDebugMode mode);
};

class CRenderingOptions
{
	// The renderer needs access to our private variables directly because capabilities have not yet been extracted
	// and thus sometimes it needs to change the rendering options without the side-effects.
	friend class CRenderer;

public:
	CRenderingOptions();
	~CRenderingOptions();

	void ReadConfigAndSetupHooks();
	void ClearHooks();

#define OPTION_DEFAULT_SETTER(NAME, TYPE) \
public: void Set##NAME(TYPE value) { m_##NAME = value; }\

#define OPTION_CUSTOM_SETTER(NAME, TYPE) \
public: void Set##NAME(TYPE value);\

#define OPTION_GETTER(NAME, TYPE)\
public: TYPE Get##NAME() const { return m_##NAME; }\

#define OPTION_DEF(NAME, TYPE)\
private: TYPE m_##NAME;

#define OPTION(NAME, TYPE)\
OPTION_DEFAULT_SETTER(NAME, TYPE); OPTION_GETTER(NAME, TYPE); OPTION_DEF(NAME, TYPE);

#define OPTION_WITH_SIDE_EFFECT(NAME, TYPE)\
OPTION_CUSTOM_SETTER(NAME, TYPE); OPTION_GETTER(NAME, TYPE); OPTION_DEF(NAME, TYPE);

	OPTION_WITH_SIDE_EFFECT(Shadows, bool);
	OPTION_WITH_SIDE_EFFECT(ShadowPCF, bool);
	OPTION_WITH_SIDE_EFFECT(Fog, bool);

	OPTION_WITH_SIDE_EFFECT(RenderPath, RenderPath);

	OPTION_WITH_SIDE_EFFECT(RenderDebugMode, RenderDebugMode);

	OPTION(WaterEffects, bool);
	OPTION(WaterFancyEffects, bool);
	OPTION(WaterRealDepth, bool);
	OPTION(WaterRefraction, bool);
	OPTION(WaterReflection, bool);

	OPTION(ShadowAlphaFix, bool);
	OPTION(Particles, bool);
	OPTION(GPUSkinning, bool);
	OPTION(Silhouettes, bool);
	OPTION(SmoothLOS, bool);
	OPTION(PostProc, bool);
	OPTION(DisplayFrustum, bool);
	OPTION(DisplayShadowsFrustum, bool);

	OPTION(RenderActors, bool);

#undef OPTION_DEFAULT_SETTER
#undef OPTION_CUSTOM_SETTER
#undef OPTION_GETTER
#undef OPTION_DEF
#undef OPTION
#undef OPTION_WITH_SIDE_EFFECT

private:
	class ConfigHooks;
	std::unique_ptr<ConfigHooks> m_ConfigHooks; // Hide this via PImpl to avoid including ConfigDB.h here.
};

extern CRenderingOptions g_RenderingOptions;

#endif // INCLUDED_RENDERINGOPTIONS
