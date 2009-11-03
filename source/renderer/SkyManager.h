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

/*
 * Sky settings and texture management
 */

#ifndef INCLUDED_SKYMANAGER
#define INCLUDED_SKYMANAGER

#include "ps/Overlay.h"

/**
 * Class SkyManager: Maintain sky settings and textures, and render the sky.
 */
class SkyManager
{
public:
	bool m_RenderSky;
	float m_HorizonHeight;

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

	/**
	 * GetSkySet(): Return the currently selected sky set name.
	 */
	inline const CStrW& GetSkySet() const {
		return m_SkySet;
	}

	/**
	 * GetSkySet(): Set the sky set name, potentially loading the textures.
	 */
	void SetSkySet(const CStrW& name);

	/**
	 * Return a sorted list of available sky sets, in a form suitable
	 * for passing to SetSkySet.
	 */
	std::vector<CStrW> GetSkySets() const;

private:
	/**
	 * load texture file from skyset and store in m_SkyTexture
	 * @param index 0..numTextures-1
	 **/
	LibError LoadSkyTexture(size_t index);

	/// Name of current skyset (a directory within art/textures/skies)
	CStrW m_SkySet;

	// Indices into m_SkyTexture
	enum
	{
		FRONT,
		BACK,
		RIGHT,
		LEFT,
		TOP,
		numTextures
	};

	// Sky textures
	Handle m_SkyTexture[numTextures];

	// Array of image names (defined in SkyManager.cpp), in the order of the IMG_ id's
	static const wchar_t* s_imageNames[numTextures];

	/// State of progressive loading (in # of loaded textures)
	size_t cur_loading_tex;
};


#endif // INCLUDED_SKYMANAGER
