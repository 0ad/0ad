#include "BrushTool.h"
#include "UIGlobals.h"
#include "HFTracer.h"
#include "NaviCam.h"
#include "TextureManager.h"
#include "Camera.h"
#include "Terrain.h"
#include "Renderer.h"
#include "ogl.h"
#include <list>

extern CCamera g_Camera;
extern CTerrain g_Terrain;

CBrushTool::CBrushTool() : m_BrushSize(1), m_LButtonDown(false), m_RButtonDown(false)
 
{
}


static void RenderTileOutline(int gx,int gz)
{
	CMiniPatch* mpatch=g_Terrain.GetTile(gx,gz);
	if (!mpatch) return;

	u32 mapSize=g_Terrain.GetVerticesPerSide();

	CVector3D V[4];
	g_Terrain.CalcPosition(gx,gz,V[0]);
	g_Terrain.CalcPosition(gx+1,gz,V[1]);
	g_Terrain.CalcPosition(gx+1,gz+1,V[2]);
	g_Terrain.CalcPosition(gx,gz+1,V[3]);
	
	glLineWidth(2);
	glColor4f(0.05f,0.95f,0.1f,0.75f);

	glBegin (GL_LINE_LOOP);
		
		for(int i = 0; i < 4; i++)
			glVertex3fv(&V[i].X);

	glEnd ();

	glColor4f (0.1f,0.9f,0.15f,0.35f);

	glBegin (GL_QUADS);
		
		for(i = 0; i < 4; i++)
			glVertex3fv(&V[i].X);

	glEnd ();
}

void CBrushTool::OnDraw()
{
	glActiveTexture (GL_TEXTURE0);
	glDisable (GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glDepthMask(0);

	int r=m_BrushSize;
	// iterate through selected patches
	for (int j=m_SelectionCentre[1]-r;j<=m_SelectionCentre[1]+r;j++) {
		for (int i=m_SelectionCentre[0]-r;i<=m_SelectionCentre[0]+r;i++) {
			RenderTileOutline(i,j);
		}
	}

	glDepthMask(1);

	glDisable(GL_BLEND);
}
	
void CBrushTool::OnLButtonDown(unsigned int flags,int px,int py) 
{
	m_LButtonDown=true;
	OnTriggerLeft();
}

void CBrushTool::OnLButtonUp(unsigned int flags,int px,int py) 
{
	m_LButtonDown=false;
}

void CBrushTool::OnRButtonDown(unsigned int flags,int px,int py) 
{
	m_RButtonDown=true;
	OnTriggerRight();
}

void CBrushTool::OnRButtonUp(unsigned int flags,int px,int py) 
{
	m_RButtonDown=false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// BuildCameraRay: calculate origin and ray direction of a ray through
// the pixel (px,py) on the screen
void CBrushTool::BuildCameraRay(int px,int py,CVector3D& origin,CVector3D& dir) 
{
	// get points on far plane of frustum in camera space
	CCamera& camera=g_NaviCam.GetCamera();
	CVector3D cPts[4];
	camera.GetCameraPlanePoints(camera.GetFarPlane(),cPts);

	// transform to world space
	CVector3D wPts[4];
	for (int i=0;i<4;i++) {
		wPts[i]=camera.m_Orientation.Transform(cPts[i]);
	}

	// get world space position of mouse point
	float dx=float(px)/float(g_Renderer.GetWidth());
	float dz=1-float(py)/float(g_Renderer.GetHeight());
	CVector3D vdx=wPts[1]-wPts[0];
	CVector3D vdz=wPts[3]-wPts[0];
	CVector3D pt=wPts[0]+(vdx*dx)+(vdz*dz);

	// copy origin
	origin=camera.m_Orientation.GetTranslation();
	// build direction
	dir=pt-origin;
	dir.Normalize();
}

void CBrushTool::OnMouseMove(unsigned int flags,int px,int py) 
{
	// build camera ray
	CVector3D rayorigin,raydir;
	BuildCameraRay(px,py,rayorigin,raydir);

	// intersect with terrain 
	CVector3D ipt;
	CHFTracer hftracer(g_Terrain.GetHeightMap(),g_Terrain.GetVerticesPerSide(),float(CELL_SIZE),HEIGHT_SCALE);
	if (hftracer.RayIntersect(rayorigin,raydir,m_SelectionCentre[0],m_SelectionCentre[1],m_SelectionPoint)) {
		// drag trigger supported?
		if (SupportDragTrigger()) {
			// yes - handle mouse button down state
			if (m_LButtonDown) {
				OnTriggerLeft();
			} else if (m_RButtonDown) {
				OnTriggerRight();
			}
		}
	}
}

