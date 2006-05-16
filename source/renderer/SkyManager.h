/**
 * =========================================================================
 * File        : SkyManager.h
 * Project     : Pyrogenesis
 * Description : Sky settings and texture management
 *
 * @author Matei Zaharia <matei@wildfiregames.com>
 * =========================================================================
 */

#ifndef SKYMANAGER_H
#define SKYMANAGER_H

#include "ps/Overlay.h"

/**
 * Class SkyManager: Maintain sky settings and textures, and render the sky.
 */
class SkyManager
{
public:
	Handle m_SkyTexture[6];

	// Indices into m_SkyTexture
	// Note: these must be kept in sync with the string constants in LoadSkyTextures; 
	//       perhaps there is a more clever way to do that.
	static const int IMG_FRONT = 0;
	static const int IMG_BACK = 1;
	static const int IMG_RIGHT = 2;
	static const int IMG_LEFT = 3;
	static const int IMG_TOP = 4;
	static const int IMG_BOTTOM = 5;

	bool m_RenderSky;

public:
	SkyManager();
	~SkyManager();

	/**
	 * LoadSkyTextures: Load sky textures from within the
	 * progressive load framework.
	 *
	 * @return 0 if loading has completed, a value from 1 to 100 (in percent of completion)
	 * if more textures need to be loaded and a negative error value on failure.
	 */
	int LoadSkyTextures();

	/**
	 * UnloadSkyTextures: Free any loaded sky textures and reset the internal state
	 * so that another call to LoadSkyTextures will begin progressive loading.
	 */
	void UnloadSkyTextures();

	/**
	 * RenderSky: Render the sky.
	 */
	void RenderSky();

private:
	/// State of progressive loading (in # of loaded textures)
	uint cur_loading_tex;
};


#endif // SKYMANAGER_H
