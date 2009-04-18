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

#ifndef INCLUDED_MINIMAP
#define INCLUDED_MINIMAP

#include "gui/GUI.h"

class CVector2D;
class CVector3D;
class CCamera;
class CTerrain;
class CUnitManager;

extern bool g_TerrainModified;

class CMiniMap : public IGUIObject
{
    GUI_OBJECT(CMiniMap)
public:
    CMiniMap();
    virtual ~CMiniMap();
protected:
    virtual void Draw();

	virtual void HandleMessage(const SGUIMessage &Message);

    // create the minimap textures
    void CreateTextures();

    // rebuild the terrain texture map
    void RebuildTerrainTexture();

    // rebuild the LOS map
    void RebuildLOSTexture();

    // destroy and free any memory and textures
    void Destroy();

	void SetCameraPos();

	void FireWorldClickEvent(int button, int clicks);

    // calculate the relative heightmap space coordinates
    // for a units world position
    CVector2D GetMapSpaceCoords(CVector3D worldPos);

    // the terrain we are mini-mapping
    const CTerrain* m_Terrain;

    // the unit manager with unit positions
    const CUnitManager* m_UnitManager;
    
	// not const: camera is moved by clicking on minimap
	CCamera* m_Camera;
	
	//Whether or not the mouse is currently down
	bool m_Clicking;	

    // minimap texture handles
    GLuint m_TerrainTexture;
    GLuint m_LOSTexture;
    
    // texture data
    u32* m_TerrainData;
    u8* m_LOSData;

    ssize_t m_Width, m_Height;

    // map size
    ssize_t m_MapSize;

    // texture size
    GLsizei m_TextureSize;

	void DrawViewRect();	// split out of Draw
};

#endif
