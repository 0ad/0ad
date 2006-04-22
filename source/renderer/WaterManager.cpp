/**
 * =========================================================================
 * File        : WaterManager.cpp
 * Project     : Pyrogenesis
 * Description : Water settings (speed, height) and texture management
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "lib/timer.h"
#include "lib/res/file/vfs.h"
#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "ps/CLogger.h"
#include "ps/Loader.h"

#include "renderer/WaterManager.h"

#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// WaterManager implementation


///////////////////////////////////////////////////////////////////
// Construction/Destruction
WaterManager::WaterManager()
{
	// water
	m_RenderWater = true;
	m_WaterHeight = 5.0f;
	m_WaterColor = CColor(0.3f, 0.35f, 0.7f, 1.0f);
	m_WaterFullDepth = 4.0f;
	m_WaterMaxAlpha = 0.85f;
	m_WaterAlphaOffset = -0.05f;
	m_SWaterTrans = 0;
	m_TWaterTrans = 0;
	m_SWaterSpeed = 0.0015f;
	m_TWaterSpeed = 0.0015f;
	m_SWaterScrollCounter = 0;
	m_TWaterScrollCounter = 0;
	m_WaterCurrentTex = 0;
	m_WaterTexTimer = 0.0;

	for (uint i = 0; i < ARRAY_SIZE(m_WaterTexture); i++)
		m_WaterTexture[i] = 0;

	cur_loading_water_tex = 0;
}

WaterManager::~WaterManager()
{
	// Cleanup if the caller messed up
	UnloadWaterTextures();
}


///////////////////////////////////////////////////////////////////
// Progressive load of water textures
int WaterManager::LoadWaterTextures()
{
	const uint num_textures = ARRAY_SIZE(m_WaterTexture);

	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 100e-3;

	while (cur_loading_water_tex < num_textures)
	{
		char waterName[PATH_MAX];
		// TODO: add a member variable and setter for this. (can't make this
		// a parameter because this function is called via delay-load code)
		static const char* const water_type = "animation2";
		snprintf(waterName, ARRAY_SIZE(waterName), "art/textures/terrain/types/water/%s/water%02d.dds", water_type, cur_loading_water_tex+1);
		Handle ht = ogl_tex_load(waterName);
		if (ht <= 0)
		{
			LOG(ERROR, LOG_CATEGORY, "LoadWaterTextures failed on \"%s\"", waterName);
			return ht;
		}
		m_WaterTexture[cur_loading_water_tex] = ht;
		RETURN_ERR(ogl_tex_upload(ht));

		cur_loading_water_tex++;
		LDR_CHECK_TIMEOUT(cur_loading_water_tex, num_textures);
	}

	return 0;
}


///////////////////////////////////////////////////////////////////
// Unload water textures
void WaterManager::UnloadWaterTextures()
{
	for(uint i = 0; i < ARRAY_SIZE(m_WaterTexture); i++)
	{
		ogl_tex_free(m_WaterTexture[i]);
		m_WaterTexture[i] = 0;
	}
	cur_loading_water_tex = 0; // so they will be reloaded if LoadWaterTextures is called again
}
