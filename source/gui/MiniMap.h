#ifndef __H_MINIMAP_H__
#define __H_MINIMAP_H__

#include "GUI.h"
#include "graphics/Terrain.h"
#include "graphics/UnitManager.h"

class CMiniMap : public IGUIObject
{
    GUI_OBJECT(CMiniMap)
public:
    CMiniMap();
    virtual ~CMiniMap();
protected:
    virtual void Draw();

    // generate the mini-map texture
    void GenerateMiniMapTexture();

    // rebuild the texture map
    void Rebuild();

    // upload the minimap texture
    void UploadTexture();

    // destroy and free any memory and textures
    void Destroy();


    // calculate the relative heightmap space coordinates
    // for a units world position
    CVector2D GetMapSpaceCoords(CVector3D worldPos);

    // the terrain we are mini-mapping
    CTerrain *m_Terrain;

    // the unit manager with unit positions
    CUnitManager *m_UnitManager;
    
    // minimap texture handle
    u32 m_Handle;
    
    // texture data
    u32 *m_Data;

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