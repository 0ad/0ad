#include "precompiled.h"

#include "MessageHandler.h"

#include "../CommandProc.h"

#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "ps/World.h"
#include "graphics/LightEnv.h"

namespace AtlasMessage {

sEnvironmentSettings GetSettings()
{
	sEnvironmentSettings s;
	
	WaterManager* wm = g_Renderer.GetWaterManager();
	s.waterheight = wm->m_WaterHeight / (65536.f * HEIGHT_SCALE);
	s.watershininess = wm->m_Shininess;
	s.waterwaviness = wm->m_Waviness;

	s.sunrotation = g_LightEnv.GetRotation();
	s.sunelevation = g_LightEnv.GetElevation();
	
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
}

BEGIN_COMMAND(SetEnvironmentSettings)

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

END_COMMAND(SetEnvironmentSettings)

QUERYHANDLER(GetEnvironmentSettings)
{
	msg->settings = GetSettings();
}

}
