/* Copyright (C) 2012 Wildfire Games.
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

#include "graphics/Texture.h"

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

	GLuint GetSkyCube() {
		return m_SkyCubeMap;
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
	void LoadSkyTextures();

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
	CTexturePtr m_SkyTexture[numTextures];

	GLuint m_SkyCubeMap;

	// Array of image names (defined in SkyManager.cpp), in the order of the IMG_ id's
	static const wchar_t* s_imageNames[numTextures];
};


#endif // INCLUDED_SKYMANAGER
