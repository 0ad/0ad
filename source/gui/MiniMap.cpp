#include "precompiled.h"

#include "gui/MiniMap.h"
#include "ps/Game.h"

#include "ogl.h"
#include "renderer/Renderer.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"

#include "Bound.h"
#include "Model.h"


static unsigned int ScaleColor(unsigned int color,float x)
{
    unsigned int r=uint(float(color & 0xff)*x);
    unsigned int g=uint(float((color>>8) & 0xff)*x);
    unsigned int b=uint(float((color>>16) & 0xff)*x);
    return (0xff000000 | r | g<<8 | b<<16);
}

static int RoundUpToPowerOf2(int x)
{
    if ((x & (x-1))==0) return x;
    int d=x;
    while (d & (d-1)) {
        d&=(d-1);
    }
    return d<<1;
}

CMiniMap::CMiniMap()
    : m_Handle(0), m_Data(NULL), m_MapSize(0), m_Terrain(0),
    m_UnitManager(0)
{
    AddSetting(GUIST_CColor,	"fov_wedge_color");
	AddSetting(GUIST_CStr,		"tooltip");
	AddSetting(GUIST_CStr,		"tooltip_style");
}

CMiniMap::~CMiniMap()
{
    Destroy();
}

void CMiniMap::Draw()
{
	// The terrain isn't actually initialized until the map is loaded, which
	// happens when the game is started
    if(GetGUI() && g_Game && g_Game->IsGameStarted())
    {
        if(!m_Handle)
            GenerateMiniMapTexture();

        g_Renderer.BindTexture(0, m_Handle);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        float texCoordMax = ((float)m_MapSize - 1) / ((float)m_TextureSize);

        glBegin(GL_QUADS);

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(m_CachedActualSize.left, m_CachedActualSize.top, GetBufferedZ());
        glTexCoord2f(texCoordMax, 0.0f);
        glVertex3f(m_CachedActualSize.right, m_CachedActualSize.top, GetBufferedZ());
        glTexCoord2f(texCoordMax, texCoordMax);
        glVertex3f(m_CachedActualSize.right, m_CachedActualSize.bottom, GetBufferedZ());
        glTexCoord2f(0.0f, texCoordMax);
        glVertex3f(m_CachedActualSize.left, m_CachedActualSize.bottom, GetBufferedZ());

        glEnd();

        float x = m_CachedActualSize.left;
        float y = m_CachedActualSize.top;
        std::vector<CUnit *> units = m_UnitManager->GetUnits();
        std::vector<CUnit *>::iterator iter = units.begin();
        CUnit *unit = NULL;
        CVector2D pos;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_POINT_SMOOTH);
        glDisable(GL_TEXTURE_2D);
        glPointSize(3.0f);
		// REMOVED: glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POINTS);
        for(; iter != units.end(); iter++)
        {
            unit = (CUnit *)(*iter);
            if(unit && unit->GetEntity())
            {
				// EDIT: John M. Mena // Set the player colour
				const SPlayerColour& colour = unit->GetEntity()->GetPlayer()->GetColour();
				glColor3f(colour.r, colour.g, colour.b);

                pos = GetMapSpaceCoords(unit->GetEntity()->m_position);

				// TODO: Investigate why player position must be reversed with map.
				// EDIT: John M. Mena // Reversed x and y addition
									  // Not quite sure what the problems is here.
                glVertex3f(x + pos.y, y + pos.x, GetBufferedZ());
            }
        }

        glEnd();

		// render view rect : John M. Mena
		// This sets up and draws the rectangle on the mini-map
		// which represents the view of the camera in the world.
		
		// Get a handle to the camera
		CCamera &g_Camera=*g_Game->GetView()->GetCamera();

		CVector3D pos3D[4];
		CVector2D pos2D[4];

		// Get the far plane corner coordinates
		g_Camera.GetCameraPlanePoints(g_Camera.GetFarPlane(), pos3D);

		// transform to the plane coords to world space
		CVector3D wPts[4];
		for (int i=0;i<4;i++) wPts[i]=g_Camera.m_Orientation.Transform(pos3D[i]);


		// TODO: Move this into the constructor
		float h=128*HEIGHT_SCALE;
		CPlane TerrainPlane;
		TerrainPlane.Set(	CVector3D( 0.0f, h, 0.0f ),
							CVector3D( float(CELL_SIZE*m_MapSize), h, 0.0f ),
							CVector3D( 0.0f, h, float(CELL_SIZE*m_MapSize) ) );
		TerrainPlane.Normalize();
		// END TODO

		// now intersect a ray from the camera through each point 
		CVector3D rayOrigin=g_Camera.m_Orientation.GetTranslation();
		CVector3D rayDir=g_Camera.m_Orientation.GetIn();

		CVector3D hitPt[4];
		for (int i=0;i<4;i++) {
			CVector3D rayDir=wPts[i]-rayOrigin;
			rayDir.Normalize();

			// get intersection point
			TerrainPlane.FindRayIntersection( rayOrigin, rayDir, &hitPt[i] );
		}

		// This is the way the ScEd converts to mini-map space.
		// When attempting to use the supplied one for this class, the lines
		// would stretch to unknown locations on the screen causing the 
		// rectangle to distort.
		// TODO: Calculate this correctly.
		// Currently the rectangle isn't drawing to the proper scale.
		float ViewRect[4][2];
		for (int i=0;i<4;i++) {
			// convert to minimap space
			float px=hitPt[i].X;
			float pz=hitPt[i].Z;
			ViewRect[i][0]=(m_CachedActualSize.GetWidth()*px/float(CELL_SIZE*m_MapSize));
			ViewRect[i][1]=(m_CachedActualSize.GetHeight()*pz/float(CELL_SIZE*m_MapSize));
		}

		// This is the alternate way that converts to mini-map space.
		//for (int i = 0; i < 4; i++)
		//	pos2D[i] = GetMapSpaceCoords(hitPt[i]);
		// END TODO

		// Enable Scissoring as to restrict the rectangle
		// to only the mini-map below by retrieving the mini-maps
		// screen coords.
		glScissor((int)m_CachedActualSize.left, 0, (int)m_CachedActualSize.right, (int)m_CachedActualSize.GetHeight());
		glEnable(GL_SCISSOR_TEST);

		glLineWidth(2);
		glColor3f(1.0f,0.3f,0.3f);

		// For some reason the coordinates need to be reversed on the x
		// and y axis or everything is backwards.  Perhaps this has something
		// to do with the location of the viewing rectangle being in the wrong
		// place?

		// Draw the viewing rectangle with the ScEd's conversion algorithm
		glBegin(GL_LINE_LOOP);
		glVertex2f(x+ViewRect[0][1],y+ViewRect[0][0]);
		glVertex2f(x+ViewRect[1][1],y+ViewRect[1][0]);
		glVertex2f(x+ViewRect[2][1],y+ViewRect[2][0]);
		glVertex2f(x+ViewRect[3][1],y+ViewRect[3][0]);
		glEnd();

		// Draw the viewing rectangle with the class' conversion algorithm
		//glBegin(GL_LINE_LOOP);
		//glVertex2f(x+pos2D[0].y, y+pos2D[0].x);
		//glVertex2f(x+pos2D[1].y, y+pos2D[1].x);
		//glVertex2f(x+pos2D[2].y, y+pos2D[2].x);
		//glVertex2f(x+pos2D[3].y, y+pos2D[3].x);
		//glEnd();

		glDisable(GL_SCISSOR_TEST);

		// Reset everything back to normal
        glPointSize(1.0f);
		glLineWidth(1.0f);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_POINT_SMOOTH);
        glEnable(GL_DEPTH_TEST);

        /*glLineWidth(2);
        glColor3f(0.4f,0.35f,0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(m_CachedActualSize.left, m_CachedActualSize.top, GetBufferedZ());
        glVertex3f(m_CachedActualSize.right, m_CachedActualSize.top, GetBufferedZ());
        glVertex3f(m_CachedActualSize.right, m_CachedActualSize.bottom, GetBufferedZ());
        glVertex3f(m_CachedActualSize.left, m_CachedActualSize.bottom, GetBufferedZ());
        glEnd();*/
    }
}

void CMiniMap::GenerateMiniMapTexture()
{
    m_Terrain = g_Game->GetWorld()->GetTerrain();
    m_UnitManager = g_Game->GetWorld()->GetUnitManager();

    m_Width = (u32)(m_CachedActualSize.right - m_CachedActualSize.left);
    m_Height = (u32)(m_CachedActualSize.bottom - m_CachedActualSize.top);

    Destroy();

    glGenTextures(1, (GLuint *)&m_Handle);
    g_Renderer.BindTexture(0, m_Handle);
    
    m_MapSize = m_Terrain->GetVerticesPerSide();
    m_TextureSize = RoundUpToPowerOf2(m_MapSize);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_TextureSize, m_TextureSize, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 0);

    m_Data = new u32[(m_MapSize - 1) * (m_MapSize - 1)];
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

    Rebuild();
}

void CMiniMap::Rebuild()
{
    u32 mapSize = m_Terrain->GetVerticesPerSide();
    u32 x = 0;
    u32 y = 0;
    u32 w = m_MapSize - 1;
    u32 h = m_MapSize - 1;

    for(u32 j = 0; j < h; j++)
    {
        u32 *dataPtr = m_Data + ((y + j) * (mapSize - 1)) + x;
        for(u32 i = 0; i < w; i++)
        {
            int hmap = ((int)m_Terrain->GetHeightMap()[(y + j) * mapSize + x + i]) >> 8;
            int val = (hmap / 3) + 170;
            CMiniPatch *mp = m_Terrain->GetTile(x + i, y + j);
            u32 color = 0;
            if(mp)
            {
                CTextureEntry *tex = mp->Tex1 ? g_TexMan.FindTexture(mp->Tex1) : 0;
                color = tex ? tex->GetBaseColor() : 0xffffffff;
            }
            else
                color = 0xffffffff;

            *dataPtr++ = ScaleColor(color, ((float)val) / 255.0f);
        }
    }

    UploadTexture();
}

void CMiniMap::UploadTexture()
{
    g_Renderer.BindTexture(0, m_Handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_BGRA_EXT, GL_UNSIGNED_BYTE, m_Data);
}

void CMiniMap::Destroy()
{
    if(m_Handle)
        glDeleteTextures(1, (GLuint *)&m_Handle);

    if(m_Data)
    {
        delete[] m_Data;
        m_Data = NULL;
    }
}

/*
 * Calefaction
 * TODO: Speed this up. There has to be some mathematical way to make
 * this more efficient. This works for now.
 */
CVector2D CMiniMap::GetMapSpaceCoords(CVector3D worldPos)
{
    u32 x = (u32)(worldPos.X / CELL_SIZE);
    // Entity's Z coordinate is really its longitudinal coordinate on the terrain
    u32 y = (u32)(worldPos.Z / CELL_SIZE);

    // Calculate map space scale
    float scaleX = float(m_Width) / float(m_MapSize - 1);
    float scaleY = float(m_Height) / float(m_MapSize - 1);
    return CVector2D(float(x) * scaleX, float(y) * scaleY);
}
