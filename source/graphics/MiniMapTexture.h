/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_MINIMAPTEXTURE
#define INCLUDED_MINIMAPTEXTURE

#include "lib/ogl.h"
#include "renderer/VertexArray.h"

class CSimulation2;
class CTerrain;

class CMiniMapTexture
{
	NONCOPYABLE(CMiniMapTexture);
public:
	CMiniMapTexture(CSimulation2& simulation);
	~CMiniMapTexture();

	/**
	 * Marks the texture as dirty if it's old enough to redraw it on Render.
	 */
	void Update(const float deltaRealTime);

	/**
	 * Redraws the texture if it's dirty.
	 */
	void Render();

	GLuint GetTexture() const { return m_FinalTexture; }

	/**
	 * @return The maximum height for unit passage in water.
	 */
	static float GetShallowPassageHeight();

private:
	void CreateTextures(const CTerrain* terrain);
	void DestroyTextures();
	void RebuildTerrainTexture(const CTerrain* terrain);
	void RenderFinalTexture();

	CSimulation2& m_Simulation;

	bool m_TerrainTextureDirty = true;
	bool m_FinalTextureDirty = true;
	double m_LastFinalTextureUpdate = 0.0;

	// minimap texture handles
	GLuint m_TerrainTexture = 0;
	GLuint m_FinalTexture = 0;

	GLuint m_FinalTextureFBO = 0;

	// texture data
	u32* m_TerrainData = nullptr;

	// map size
	ssize_t m_MapSize = 0;

	// texture size
	GLsizei m_TextureSize = 0;

	// Maximal water height to allow the passage of a unit (for underwater shallows).
	float m_ShallowPassageHeight = 0.0f;
	float m_WaterHeight = 0.0f;

	VertexIndexArray m_IndexArray;
	VertexArray m_VertexArray;
	VertexArray::Attribute m_AttributePos;
	VertexArray::Attribute m_AttributeColor;

	size_t m_EntitiesDrawn = 0;

	double m_PingDuration = 25.0;
	double m_HalfBlinkDuration = 0.0;
	double m_NextBlinkTime = 0.0;
	bool m_BlinkState = false;
};

#endif // INCLUDED_MINIMAPTEXTURE
