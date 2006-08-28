#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "graphics/LightEnv.h"
#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SkyManager.h"
#include "renderer/WaterManager.h"

namespace AtlasMessage {

sEnvironmentSettings GetSettings()
{
	sEnvironmentSettings s;
	
	WaterManager* wm = g_Renderer.GetWaterManager();
	s.waterheight = wm->m_WaterHeight / (65536.f * HEIGHT_SCALE);
	s.watershininess = wm->m_Shininess;
	s.waterwaviness = wm->m_Waviness;

	float sunrotation = g_LightEnv.GetRotation();
	if (sunrotation > PI)
		sunrotation -= PI*2;
	s.sunrotation = sunrotation;
	s.sunelevation = g_LightEnv.GetElevation();

	s.skyset = g_Renderer.GetSkyManager()->GetSkySet();

#define COLOUR(A, B) A = Colour(B.X*255, B.Y*255, B.Z*255)
	COLOUR(s.suncolour, g_LightEnv.m_SunColor);
	COLOUR(s.terraincolour, g_LightEnv.m_TerrainAmbientColor);
	COLOUR(s.unitcolour, g_LightEnv.m_UnitsAmbientColor);
#undef COLOUR

	return s;
}

void SetSettings(const sEnvironmentSettings& s)
{
	WaterManager* wm = g_Renderer.GetWaterManager();
	wm->m_WaterHeight = s.waterheight * (65536.f * HEIGHT_SCALE);
	wm->m_Shininess = s.watershininess;
	wm->m_Waviness = s.waterwaviness;

	g_LightEnv.SetRotation(s.sunrotation);
	g_LightEnv.SetElevation(s.sunelevation);

	CStrW skySet = *s.skyset;
	if (skySet.length() == 0)
		skySet = L"default";
	g_Renderer.GetSkyManager()->SetSkySet(skySet);

#define COLOUR(A, B) B = RGBColor(A->r/255.f, A->g/255.f, A->b/255.f)
	COLOUR(s.suncolour, g_LightEnv.m_SunColor);
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
