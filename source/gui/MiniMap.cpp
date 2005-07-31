#include "precompiled.h"

#include "gui/MiniMap.h"
#include "ps/Game.h"

#include <math.h>
#include "ogl.h"
#include "renderer/Renderer.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "graphics/Unit.h"

#include "Bound.h"
#include "Model.h"

#define VR_XMOVE (24)
#define VR_YMOVE (24.5)

extern bool mouseButtons[5];
extern int g_mouse_x, g_mouse_y;
bool HasClicked=false;

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

		float z = GetBufferedZ();

		glBegin(GL_QUADS);

		// Draw the main textured quad
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(m_CachedActualSize.left, m_CachedActualSize.bottom, z);
		glTexCoord2f(texCoordMax, 0.0f);
		glVertex3f(m_CachedActualSize.right, m_CachedActualSize.bottom, z);
		glTexCoord2f(texCoordMax, texCoordMax);
		glVertex3f(m_CachedActualSize.right, m_CachedActualSize.top, z);
		glTexCoord2f(0.0f, texCoordMax);
		glVertex3f(m_CachedActualSize.left, m_CachedActualSize.top, z);

		glEnd();

		float x = m_CachedActualSize.left;
		float y = m_CachedActualSize.bottom;
		const std::vector<CUnit *> &units = m_UnitManager->GetUnits();
		std::vector<CUnit *>::const_iterator iter = units.begin();
		CUnit *unit = NULL;
		CVector2D pos;

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_POINT_SMOOTH);
		glDisable(GL_TEXTURE_2D);
		glPointSize(3.0f);
		// REMOVED: glColor3f(1.0f, 1.0f, 1.0f);
		glBegin(GL_POINTS);
		for(; iter != units.end(); ++iter)
		{
			unit = (CUnit *)(*iter);
			if(unit && unit->GetEntity())
			{
				// Set the player colour
				const SPlayerColour& colour = unit->GetEntity()->GetPlayer()->GetColour();
				glColor3f(colour.r, colour.g, colour.b);

				pos = GetMapSpaceCoords(unit->GetEntity()->m_position);

				glVertex3f(x + pos.x, y - pos.y, z);
			}
		}

		glEnd();

		
		
		
//================================================================	
              //INTERACTIVE MINIMAP STARTS
		

		//Get Camera handle
		CCamera &g_Camera=*g_Game->GetView()->GetCamera();
		
		//Check for a click
		if(mouseButtons[0]==true)
		{  
			HasClicked=true; 
		}
		
		//Check to see if left button is false (meaning it's been lifted) 
		if (mouseButtons[0]==false && HasClicked==true)

		{

			//Is cursor inside Minimap boundaries? 
			if(g_mouse_x > m_CachedActualSize.left && g_mouse_x < m_CachedActualSize.right
				&& g_mouse_y > m_CachedActualSize.top && g_mouse_y < m_CachedActualSize.bottom)
				{
					
					CVector3D mm_CurrentPos=g_Camera.m_Orientation.GetTranslation();
					
					//X and Z according to proportion of mouse position and minimap
						CVector3D mm_Destination;
					mm_Destination.X=(CELL_SIZE*m_MapSize)*
						((g_mouse_x-m_CachedActualSize.left)/m_CachedActualSize.GetWidth());
					mm_Destination.Y=mm_CurrentPos.Y;
					mm_Destination.Z=(CELL_SIZE*m_MapSize)*
					((m_CachedActualSize.bottom-g_mouse_y)/m_CachedActualSize.GetHeight());

				//Extra constants for compensating for inaccurate conversion
				//Decimal multiplied for ratio of constant/zoom so it works on all zooms
					mm_Destination.X+=.7*mm_Destination.Y;
					mm_Destination.Z-=.7*mm_Destination.Y;
					
					//Move lower left side of frustum to mouse
					g_Camera.m_Orientation._14=mm_Destination.X;
					g_Camera.m_Orientation._34=mm_Destination.Z;
									
					g_Camera.UpdateFrustum();
					
				}
			HasClicked=false;
		}
							//END OF INTERACTIVE MINIMAP
	//====================================================================
		
		
		// render view rect : John M. Mena
		// This sets up and draws the rectangle on the mini-map
		// which represents the view of the camera in the world.
		
		// Get a handle to the camera
		

		//Get correct world coordinates based off corner of screen start 
		//at Bottom Left and going CW
		CVector3D hitPt[4];
		hitPt[0]=g_Camera.GetWorldCoordinates(0,g_Renderer.GetHeight());
		hitPt[1]=g_Camera.GetWorldCoordinates(g_Renderer.GetWidth(),g_Renderer.GetHeight());
		hitPt[2]=g_Camera.GetWorldCoordinates(g_Renderer.GetWidth(),0);
		hitPt[3]=g_Camera.GetWorldCoordinates(0,0);
		

		float ViewRect[4][2];
		for (int i=0;i<4;i++) {
			// convert to minimap space
			float px=hitPt[i].X;
			float pz=hitPt[i].Z;
			ViewRect[i][0]=(m_CachedActualSize.GetWidth()*px/float(CELL_SIZE*m_MapSize));
			ViewRect[i][1]=(m_CachedActualSize.GetHeight()*pz/float(CELL_SIZE*m_MapSize));
		}

	

		// Enable Scissoring as to restrict the rectangle
		// to only the mini-map below by retrieving the mini-maps
		// screen coords.
		
		glScissor((int)m_CachedActualSize.left, 0, (int)m_CachedActualSize.right, (int)m_CachedActualSize.GetHeight());
		glEnable(GL_SCISSOR_TEST);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(2);
		glColor3f(1.0f,0.3f,0.3f);
		
		
		// Draw the viewing rectangle with the ScEd's conversion algorithm
		glBegin(GL_LINE_LOOP);
		glVertex2f(x+ViewRect[0][0], y-ViewRect[0][1]);
		glVertex2f(x+ViewRect[1][0], y-ViewRect[1][1]);
		glVertex2f(x+ViewRect[2][0], y-ViewRect[2][1]);
		glVertex2f(x+ViewRect[3][0], y-ViewRect[3][1]);
		glEnd();


		
		glDisable(GL_SCISSOR_TEST);

		// Reset everything back to normal
		glPointSize(1.0f);
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_POINT_SMOOTH);
		glEnable(GL_DEPTH_TEST);
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
