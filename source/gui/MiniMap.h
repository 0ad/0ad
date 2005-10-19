#ifndef __H_MINIMAP_H__
#define __H_MINIMAP_H__

#include "GUI.h"
#include "Vector2D.h"
#include "Vector3D.h"
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

    // create the minimap textures
    void CreateTextures();

    // rebuild the terrain texture map
    void RebuildTerrainTexture();

    // rebuild the LOS map
    void RebuildLOSTexture();

    // destroy and free any memory and textures
    void Destroy();


    // calculate the relative heightmap space coordinates
    // for a units world position
    CVector2D GetMapSpaceCoords(CVector3D worldPos);

    // the terrain we are mini-mapping
    CTerrain *m_Terrain;

    // the unit manager with unit positions
    CUnitManager *m_UnitManager;
    
    // minimap texture handles
    u32 m_TerrainTexture;
    u32 m_LOSTexture;
    
    // texture data
    u32 *m_TerrainData;
    u8 *m_LOSData;

    // width
    u32 m_Width;

    // height
    u32 m_Height;

    // map size
    u32 m_MapSize;

    // texture size
    u32 m_TextureSize;
};

#endif
