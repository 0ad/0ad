#include "Renderer.h"
#include "Matrix3D.h"
#include "Camera.h"

#include "types.h"
#include "ogl.h"
#include "tex.h"

#define		RENDER_STAGE_BASE		(1)
#define		RENDER_STAGE_TRANS		(2)

bool g_WireFrame = false;
unsigned int g_FrameCounter = 0;

CRenderer::CRenderer ()
{
	m_Timer = 0;
	m_CurrentSeason = 0;
}

CRenderer::~CRenderer ()
{
}

	
bool CRenderer::Initialize (int width, int height, int depth)
{
	m_Width = width;
	m_Height = height;
	m_Depth = depth;
	return true;
}

void CRenderer::Shutdown ()
{

}
/*
struct Tile
{
	u32 pri_tex : 5;
	u32 sec_tex : 5;
	u32 alpha_map : 6;
};


void render_terrain()
{
	CMatrix3D view = camera->m_Orientation.GetTranspose();
	CMatrix3D proj = camera->GetProjection();

	float gl_view[16] = {view._11, view._21, view._31, view._41,
						 view._12, view._22, view._32, view._42,
						 view._13, view._23, view._33, view._43,
						 view._14, view._24, view._34, view._44};

	float gl_proj[16] = {proj._11, proj._21, proj._31, proj._41,
						 proj._12, proj._22, proj._32, proj._42,
						 proj._13, proj._23, proj._33, proj._43,
						 proj._14, proj._24, proj._34, proj._44};

	glMatrixMode (GL_MODELVIEW);
	glLoadMatrixf (gl_view);

	glMatrixMode (GL_PROJECTION);
	glLoadMatrixf (gl_proj);

	SViewPort vp = camera->GetViewPort();
	glViewport (vp.m_X, vp.m_Y, vp.m_Width, vp.m_Height);

	if (g_WireFrame)
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);


	for (int j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			if (camera->GetFustum().IsBoxVisible (CVector3D(0,0,0), terrain->m_Patches[j][i].m_Bounds))
				terrain->m_Patches[j][i].m_LastVisFrame = g_FrameCounter;
		}
	}

	for (j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			if (terrain->m_Patches[j][i].m_LastVisFrame == g_FrameCounter)
				render_patch(&terrain->m_Patches[j][i]);
		}
	}
}
*/
void CRenderer::RenderTerrain (CTerrain *terrain, CCamera *camera)
{
//	m_Timer += 0.001f;

	if (m_Timer > 1.0f)
	{
		m_Timer = 0;
		
		if (m_CurrentSeason == 0)
			m_CurrentSeason = 1;
		else
			m_CurrentSeason = 0;
	}

	CMatrix3D view = camera->m_Orientation.GetTranspose();
	CMatrix3D proj = camera->GetProjection();

	float gl_view[16] = {view._11, view._21, view._31, view._41,
						 view._12, view._22, view._32, view._42,
						 view._13, view._23, view._33, view._43,
						 view._14, view._24, view._34, view._44};

	float gl_proj[16] = {proj._11, proj._21, proj._31, proj._41,
						 proj._12, proj._22, proj._32, proj._42,
						 proj._13, proj._23, proj._33, proj._43,
						 proj._14, proj._24, proj._34, proj._44};


	glMatrixMode (GL_MODELVIEW);
	glLoadMatrixf (gl_view);

	glMatrixMode (GL_PROJECTION);
	glLoadMatrixf (gl_proj);

	SViewPort vp = camera->GetViewPort();
	//glViewport (vp.m_X, vp.m_Y, vp.m_Width, vp.m_Height);

	if (g_WireFrame)
		glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);


	for (int j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			if (camera->GetFustum().IsBoxVisible (CVector3D(0,0,0), terrain->m_Patches[j][i].m_Bounds))
				terrain->m_Patches[j][i].m_LastVisFrame = g_FrameCounter;
		}
	}

	for (j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			if (terrain->m_Patches[j][i].m_LastVisFrame == g_FrameCounter)
				RenderPatchBase (&terrain->m_Patches[j][i]);
		}
	}


	for (j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			if (terrain->m_Patches[j][i].m_LastVisFrame == g_FrameCounter)
				RenderPatchTrans (&terrain->m_Patches[j][i]);

		}
	}
}

void CRenderer::RenderPatchBase (CPatch *patch)
{
	CMiniPatch *MPatch, *MPCurrent;

	float StartU, StartV;
	

	for (int j=0; j<16; j++)
	{
		for (int i=0; i<16; i++)
		{
			MPatch = &(patch->m_MiniPatches[j][i]);

			if (MPatch->m_LastRenderedFrame == g_FrameCounter)
				continue;

			glActiveTexture (GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);

tex_bind(MPatch->Tex1);

			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

/////////////////////////////////////
			glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

			glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
			glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
			glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
			glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
			glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

			glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
			glTexEnvf (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
			glTexEnvf (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
/////////////////////////////////////

			StartU = 0.125f * (float)(i%8);
			StartV = 0.125f * (float)(j%8);

			float tu[2], tv[2];
			tu[0] = tu[1] = StartU;
			tv[0] = StartV+0.125f;
			tv[1] = StartV;

			MPCurrent = MPatch;
			glBegin (GL_TRIANGLE_STRIP);

			int start = 0;

			while (MPCurrent)
			{
				for (int x=start; x<2; x++)
				{
					int v1 = MAP_SIZE + x;
					int v2 = x;

					float factor = m_Timer;
					if (m_CurrentSeason == 1)
						factor = 1.0f - factor;

					float color1[3] = {MPCurrent->m_pVertices[v1].m_Color[0][0]*factor + MPCurrent->m_pVertices[v1].m_Color[1][0]*(1.0f-factor),
									   MPCurrent->m_pVertices[v1].m_Color[0][1]*factor + MPCurrent->m_pVertices[v1].m_Color[1][1]*(1.0f-factor),
									   MPCurrent->m_pVertices[v1].m_Color[0][2]*factor + MPCurrent->m_pVertices[v1].m_Color[1][2]*(1.0f-factor)};

					float color2[3] = {MPCurrent->m_pVertices[v2].m_Color[0][0]*factor + MPCurrent->m_pVertices[v2].m_Color[1][0]*(1.0f-factor),
									   MPCurrent->m_pVertices[v2].m_Color[0][1]*factor + MPCurrent->m_pVertices[v2].m_Color[1][1]*(1.0f-factor),
									   MPCurrent->m_pVertices[v2].m_Color[0][2]*factor + MPCurrent->m_pVertices[v2].m_Color[1][2]*(1.0f-factor)};

					glTexCoord2f (tu[0], tv[0]);

					if (g_HillShading)
						glColor3f (color1[0],color1[1],color1[2]);
					else
						glColor3f (1,1,1);

					glVertex3f (MPCurrent->m_pVertices[v1].m_Position.X,
								MPCurrent->m_pVertices[v1].m_Position.Y,
								MPCurrent->m_pVertices[v1].m_Position.Z);

					glTexCoord2f (tu[1], tv[1]);

					if (g_HillShading)
						glColor3f (color2[0],color2[1],color2[2]);
					else
						glColor3f (1,1,1);
					
					glVertex3f (MPCurrent->m_pVertices[v2].m_Position.X,
								MPCurrent->m_pVertices[v2].m_Position.Y,
								MPCurrent->m_pVertices[v2].m_Position.Z);

					tu[0]+=0.125f;
					tu[1]+=0.125f;
				}

				MPCurrent->m_LastRenderedFrame = g_FrameCounter;
				MPCurrent->m_RenderStage = RENDER_STAGE_BASE;

				if (!MPCurrent->m_pRightNeighbor)
					break;
				else
				{
					if (MPCurrent->m_pRightNeighbor->Tex1 != MPCurrent->Tex1 ||
						MPCurrent->m_pRightNeighbor->m_pParrent->m_LastVisFrame != g_FrameCounter)
						break;
				}

				MPCurrent = MPCurrent->m_pRightNeighbor;
				start = 1;
			}

			glEnd ();
		}
	}
}

void CRenderer::RenderPatchTrans (CPatch *patch)
{
	CMiniPatch *MPatch, *MPCurrent;

	float StartU, StartV;
	
	glEnable (GL_BLEND);
	glDepthFunc (GL_EQUAL);
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	for (int j=0; j<16; j++)
	{
		for (int i=0; i<16; i++)
		{
			MPatch = &(patch->m_MiniPatches[j][i]);

			if (MPatch->m_LastRenderedFrame == g_FrameCounter &&
				MPatch->m_RenderStage == RENDER_STAGE_TRANS)
				continue;

			//now for transition
			if (MPatch->Tex2 && MPatch->m_AlphaMap)
			{

				glActiveTexture (GL_TEXTURE0);
				glEnable(GL_TEXTURE_2D);

tex_bind(MPatch->Tex2);

				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

/////////////////////////////////////
				glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
					
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
/////////////////////////////////////
				

				glActiveTexture (GL_TEXTURE1);
				glEnable(GL_TEXTURE_2D);
tex_bind(MPatch->m_AlphaMap);				
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
/////////////////////////////////////
				glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
				
				glTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
				glTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
				glTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
/////////////////////////////////////

				StartU = 0.125f * (float)(i%8);
				StartV = 0.125f * (float)(j%8);

				float tu[2], tv[2];
				tu[0] = tu[1] = StartU;
				tv[0] = StartV+0.125f;
				tv[1] = StartV;
				
				glBegin (GL_TRIANGLE_STRIP);
				MPCurrent = MPatch;

				int start = 0;

				while (MPCurrent)
				{
					for (int x=start; x<2; x++)
					{
						int v1 = MAP_SIZE + x;
						int v2 = x;

						float factor = m_Timer;
						if (m_CurrentSeason == 1)
							factor = 1.0f - factor;
							
						float color1[3] = {MPCurrent->m_pVertices[v1].m_Color[0][0]*factor + MPCurrent->m_pVertices[v1].m_Color[1][0]*(1.0f-factor),
										   MPCurrent->m_pVertices[v1].m_Color[0][1]*factor + MPCurrent->m_pVertices[v1].m_Color[1][1]*(1.0f-factor),
										   MPCurrent->m_pVertices[v1].m_Color[0][2]*factor + MPCurrent->m_pVertices[v1].m_Color[1][2]*(1.0f-factor)};

						float color2[3] = {MPCurrent->m_pVertices[v2].m_Color[0][0]*factor + MPCurrent->m_pVertices[v2].m_Color[1][0]*(1.0f-factor),
										   MPCurrent->m_pVertices[v2].m_Color[0][1]*factor + MPCurrent->m_pVertices[v2].m_Color[1][1]*(1.0f-factor),
										   MPCurrent->m_pVertices[v2].m_Color[0][2]*factor + MPCurrent->m_pVertices[v2].m_Color[1][2]*(1.0f-factor)};

						glMultiTexCoord2f (GL_TEXTURE0_ARB, tu[0], tv[0]);
						glMultiTexCoord2f (GL_TEXTURE1_ARB, tu[0]*2, tv[0]*2);
						
						if (g_HillShading)
							glColor3f (color1[0],color1[1],color1[2]);
						else
							glColor3f (1,1,1);

						glVertex3f (MPCurrent->m_pVertices[v1].m_Position.X,
									MPCurrent->m_pVertices[v1].m_Position.Y,
									MPCurrent->m_pVertices[v1].m_Position.Z);

						glMultiTexCoord2f (GL_TEXTURE0_ARB, tu[1], tv[1]);
						glMultiTexCoord2f (GL_TEXTURE1_ARB, tu[1]*2, tv[1]*2);
						
						if (g_HillShading)
							glColor3f (color2[0],color2[1],color2[2]);
						else
							glColor3f (1,1,1);
						
						glVertex3f (MPCurrent->m_pVertices[v2].m_Position.X,
									MPCurrent->m_pVertices[v2].m_Position.Y,
									MPCurrent->m_pVertices[v2].m_Position.Z);

						tu[0]+=0.125f;
						tu[1]+=0.125f;
					}

					MPCurrent->m_LastRenderedFrame = g_FrameCounter;
					MPCurrent->m_RenderStage = RENDER_STAGE_TRANS;

					if (!MPCurrent->m_pRightNeighbor)
						break;
					else
					{
						if (MPCurrent->m_pRightNeighbor->Tex2 != MPCurrent->Tex2 ||
						    MPCurrent->m_pRightNeighbor->m_AlphaMap != MPCurrent->m_AlphaMap ||
							MPCurrent->m_pRightNeighbor->m_pParrent->m_LastVisFrame != g_FrameCounter)
							break;
					}

					MPCurrent = MPCurrent->m_pRightNeighbor;
					start=1;
				}

				glEnd ();
			}
		}
	}

	glDepthFunc (GL_LEQUAL);
	glDisable (GL_BLEND);
	glActiveTexture (GL_TEXTURE1);
glDisable(GL_TEXTURE_2D);
}

void CRenderer::RenderTileOutline (CMiniPatch *mpatch)
{
if(!mpatch->m_pVertices)
return;

	glActiveTexture (GL_TEXTURE0);
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);
	glLineWidth (4);

	STerrainVertex V[4];
	V[0] = mpatch->m_pVertices[0];
	V[1] = mpatch->m_pVertices[1];
	V[2] = mpatch->m_pVertices[MAP_SIZE*1 + 1];
	V[3] = mpatch->m_pVertices[MAP_SIZE*1];
	
	glColor3f (0,1.0f,0);

	glBegin (GL_LINE_LOOP);
		
		for(int i = 0; i < 4; i++)
			glVertex3fv(&V[i].m_Position.X);

	glEnd ();

	glEnable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);
}
