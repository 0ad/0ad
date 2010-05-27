/* Copyright (C) 2009 Wildfire Games.
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

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/LightEnv.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"

namespace AtlasMessage {

sEnvironmentSettings GetSettings()
{
	sEnvironmentSettings s;

	CmpPtr<ICmpWaterManager> cmpWaterMan(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	debug_assert(!cmpWaterMan.null());

	s.waterheight = cmpWaterMan->GetExactWaterLevel(0, 0) / (65536.f * HEIGHT_SCALE);

	WaterManager* wm = g_Renderer.GetWaterManager();
	s.watershininess = wm->m_Shininess;
	s.waterwaviness = wm->m_Waviness;
	s.watermurkiness = wm->m_Murkiness;
	s.waterreflectiontintstrength = wm->m_ReflectionTintStrength;

	// CColor colours
#define COLOUR(A, B) A = Colour((int)(B.r*255), (int)(B.g*255), (int)(B.b*255))
	COLOUR(s.watercolour, wm->m_WaterColor);
	COLOUR(s.watertint, wm->m_WaterTint);
	COLOUR(s.waterreflectiontint, wm->m_ReflectionTint);
#undef COLOUR

	float sunrotation = g_LightEnv.GetRotation();
	if (sunrotation > (float)M_PI)
		sunrotation -= (float)M_PI*2;
	s.sunrotation = sunrotation;
	s.sunelevation = g_LightEnv.GetElevation();

	s.skyset = g_Renderer.GetSkyManager()->GetSkySet();

	// RGBColor (CVector3D) colours
#define COLOUR(A, B) A = Colour((int)(B.X*255), (int)(B.Y*255), (int)(B.Z*255))
	s.sunoverbrightness = MaxComponent(g_LightEnv.m_SunColor);
	// clamp color to [0..1] before packing into u8 triplet
	if(s.sunoverbrightness > 1.0f)
		g_LightEnv.m_SunColor *= 1.0/s.sunoverbrightness;	// (there's no operator/=)
	// no component was above 1.0, so reset scale factor (don't want to darken)
	else
		s.sunoverbrightness = 1.0f;
	COLOUR(s.suncolour, g_LightEnv.m_SunColor);
	COLOUR(s.terraincolour, g_LightEnv.m_TerrainAmbientColor);
	COLOUR(s.unitcolour, g_LightEnv.m_UnitsAmbientColor);
#undef COLOUR

	return s;
}

void SetSettings(const sEnvironmentSettings& s)
{
	CmpPtr<ICmpWaterManager> cmpWaterMan(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	debug_assert(!cmpWaterMan.null());

	cmpWaterMan->SetWaterLevel(entity_pos_t::FromFloat(s.waterheight * (65536.f * HEIGHT_SCALE)));

	WaterManager* wm = g_Renderer.GetWaterManager();
	wm->m_Shininess = s.watershininess;
	wm->m_Waviness = s.waterwaviness;
	wm->m_Murkiness = s.watermurkiness;
	wm->m_ReflectionTintStrength = s.waterreflectiontintstrength;

#define COLOUR(A, B) B = CColor(A->r/255.f, A->g/255.f, A->b/255.f, 1.f)
	COLOUR(s.watercolour, wm->m_WaterColor);
	COLOUR(s.watertint, wm->m_WaterTint);
	COLOUR(s.waterreflectiontint, wm->m_ReflectionTint);
#undef COLOUR

	g_LightEnv.SetRotation(s.sunrotation);
	g_LightEnv.SetElevation(s.sunelevation);

	CStrW skySet = *s.skyset;
	if (skySet.length() == 0)
		skySet = L"default";
	g_Renderer.GetSkyManager()->SetSkySet(skySet);

#define COLOUR(A, B) B = RGBColor(A->r/255.f, A->g/255.f, A->b/255.f)
	COLOUR(s.suncolour, g_LightEnv.m_SunColor);
	g_LightEnv.m_SunColor *= s.sunoverbrightness;
	COLOUR(s.terraincolour, g_LightEnv.m_TerrainAmbientColor);
	COLOUR(s.unitcolour, g_LightEnv.m_UnitsAmbientColor);
#undef COLOUR
}

BEGIN_COMMAND(SetEnvironmentSettings)
{
	sEnvironmentSettings m_OldSettings, m_NewSettings;

	void Do()
	{
		m_OldSettings = GetSettings();
		m_NewSettings = msg->settings;
		Redo();
	}

	void Redo()
	{
		SetSettings(m_NewSettings);
	}

	void Undo()
	{
		SetSettings(m_OldSettings);
	}

	void MergeIntoPrevious(cSetEnvironmentSettings* prev)
	{
		prev->m_NewSettings = m_NewSettings;
	}
};
END_COMMAND(SetEnvironmentSettings)

QUERYHANDLER(GetEnvironmentSettings)
{
	msg->settings = GetSettings();
}

QUERYHANDLER(GetSkySets)
{
	std::vector<CStrW> skies = g_Renderer.GetSkyManager()->GetSkySets();
	msg->skysets = std::vector<std::wstring>(skies.begin(), skies.end());
}

}
