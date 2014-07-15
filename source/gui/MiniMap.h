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

#ifndef INCLUDED_MINIMAP
#define INCLUDED_MINIMAP

#include "gui/GUI.h"
#include "renderer/VertexArray.h"


class CCamera;
class CTerrain;

class CMiniMap : public IGUIObject
{
	GUI_OBJECT(CMiniMap)
public:
	CMiniMap();
	virtual ~CMiniMap();
protected:
	virtual void Draw();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage &Message);

	/**
	 * @see IGUIObject#MouseOver()
	 */
	virtual bool MouseOver();

	// create the minimap textures
	void CreateTextures();

	// rebuild the terrain texture map
	void RebuildTerrainTexture();

	// destroy and free any memory and textures
	void Destroy();

	void SetCameraPos();

	void FireWorldClickEvent(int button, int clicks);

	// the terrain we are mini-mapping
	const CTerrain* m_Terrain;

	const CCamera* m_Camera;

	//Whether or not the mouse is currently down
	bool m_Clicking;

	// minimap texture handles
	GLuint m_TerrainTexture;

	// texture data
	u32* m_TerrainData;

	// whether we need to regenerate the terrain texture
	bool m_TerrainDirty;

	ssize_t m_Width, m_Height;

	// map size
	ssize_t m_MapSize;

	// texture size
	GLsizei m_TextureSize;

	// 1.f if map is circular or 1.414f if square (to shrink it inside the circle)
	float m_MapScale;
	
	// maximal water height to allow the passage of a unit (for underwater shallows).
	float m_ShallowPassageHeight;

	void DrawTexture(CShaderProgramPtr shader, float coordMax, float angle, float x, float y, float x2, float y2, float z);

	void DrawViewRect(CMatrix3D transform);

	void GetMouseWorldCoordinates(float& x, float& z);

	float GetAngle();

	VertexIndexArray m_IndexArray;
	VertexArray m_VertexArray;
	VertexArray::Attribute m_AttributePos;
	VertexArray::Attribute m_AttributeColor;

	size_t m_EntitiesDrawn;

	double m_PingDuration;
	double m_HalfBlinkDuration;
	double m_NextBlinkTime;
	bool m_BlinkState;
};

#endif
