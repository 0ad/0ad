//----------------------------------------------------------------
//
// Name:		Renderer.cpp
// Last Update: 25/11/03
// Author:		Rich Cross
// Contact:		rich@0ad.wildfiregames.com
//
// Description: OpenGL renderer class; a higher level interface
//	on top of OpenGL to handle rendering the basic visual games 
//	types - terrain, models, sprites, particles etc
//----------------------------------------------------------------

#include "Renderer.h"
#include "Terrain.h"
#include "Matrix3D.h"
#include "Camera.h"
#include "Texture.h"

#include "Model.h"
#include "ModelDef.h"

#include "types.h"
#include "ogl.h"
#include "res/res.h"

#define		RENDER_STAGE_BASE		(1)
#define		RENDER_STAGE_TRANS		(2)

CRenderer::CRenderer ()
{
	m_Width=0;
	m_Height=0;
	m_Depth=0;
	m_FrameCounter=0;
	m_TerrainMode=FILL;
}

CRenderer::~CRenderer ()
{
}

	
bool CRenderer::Open(int width, int height, int depth)
{
	m_Width = width;
	m_Height = height;
	m_Depth = depth;

	// setup default state
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f,0.0f,0.0f,0.0f);

	return true;
}

void CRenderer::Close()
{
}

// resize renderer view
void CRenderer::Resize(int width,int height)
{
	m_Width = width;
	m_Height = height;
}

// signal frame start
void CRenderer::BeginFrame()
{
	// bump frame counter
	m_FrameCounter++;

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// force rendering of any batched objects
void CRenderer::FlushFrame()
{	
	unsigned i;

	// render base terrain
	if (m_TerrainMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}

	for (i=0;i<m_TerrainPatches.size();++i) {
		RenderPatchBase(m_TerrainPatches[i].m_Object);
	}
	for (i=0;i<m_TerrainPatches.size();++i) {
		RenderPatchTrans(m_TerrainPatches[i].m_Object);
	}

	if (m_TerrainMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}

	// render models
	for (i=0;i<m_Models.size();++i) {
		RenderModel(m_Models[i]);
	}

	// empty lists
	m_TerrainPatches.clear();
	m_Models.clear();
}

// signal frame end : implicitly flushes batched objects 
void CRenderer::EndFrame()
{
	FlushFrame();
}

void CRenderer::SetCamera(CCamera& camera)
{
	CMatrix3D view = camera.m_Orientation.GetTranspose();
	CMatrix3D proj = camera.GetProjection();

	float gl_view[16] = {view._11, view._21, view._31, view._41,
						 view._12, view._22, view._32, view._42,
						 view._13, view._23, view._33, view._43,
						 view._14, view._24, view._34, view._44};

	float gl_proj[16] = {proj._11, proj._21, proj._31, proj._41,
						 proj._12, proj._22, proj._32, proj._42,
						 proj._13, proj._23, proj._33, proj._43,
						 proj._14, proj._24, proj._34, proj._44};


	glMatrixMode (GL_PROJECTION);
	glLoadMatrixf (gl_proj);

	glMatrixMode (GL_MODELVIEW);
	glLoadMatrixf (gl_view);

	const SViewPort& vp = camera.GetViewPort();
	glViewport (vp.m_X, vp.m_Y, vp.m_Width, vp.m_Height);
}

void CRenderer::Submit(CPatch* patch)
{
	SSubmission<CPatch*> sub;
	patch->m_LastVisFrame=m_FrameCounter;
	sub.m_Object=patch;
	m_TerrainPatches.push_back(sub);
}

void CRenderer::Submit(CModel* model,CMatrix3D* transform)
{
	SSubmission<CModel*> sub;
	sub.m_Object=model;
	sub.m_Transform=transform;
	m_Models.push_back(sub);
}

void CRenderer::Submit(CSprite* sprite,CMatrix3D* transform)
{
}

void CRenderer::Submit(CParticleSys* psys,CMatrix3D* transform)
{
}

void CRenderer::Submit(COverlay* overlay)
{
}



/*
void CRenderer::RenderTileOutline (CMiniPatch *mpatch)
{
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
*/

void CRenderer::RenderModel(SSubmission<CModel*>& modelsub)
{
	glPushMatrix();
	glMultMatrixf(modelsub.m_Transform->_data);

	SetTexture(0,modelsub.m_Object->GetTexture());
	glColor3f(1.0f,1.0f,1.0f);

	CModel* mdl=(CModel*) modelsub.m_Object;
	CModelDef* mdldef=(CModelDef*) mdl->GetModelDef();
	
	glBegin(GL_TRIANGLES);
	for (int fi=0; fi<mdldef->GetNumFaces(); fi++)
	{
		SModelFace *pFace = &mdldef->GetFaces()[fi];

		for (int vi=0; vi<3; vi++)
		{
			SModelVertex *pVertex = &mdldef->GetVertices()[pFace->m_Verts[vi]];
			CVector3D Coord = mdl->GetBonePoses()[pVertex->m_Bone].Transform(pVertex->m_Coords);

			glTexCoord2f (pVertex->m_U, pVertex->m_V);

			glVertex3f (Coord.X, Coord.Y, Coord.Z);
		}
	}
	glEnd();

	glPopMatrix();
}

// try and load the given texture
bool CRenderer::LoadTexture(CTexture* texture)
{
	Handle h=texture->GetHandle();
	if (h) {
		// already tried to load this texture, nothing to do here - just return accord to whether this
		// is a valid handle
		return h==0xfffffff ? true : false;
	} else {
		h=tex_load(texture->GetName());
		if (!h) {
			texture->SetHandle(0xffffffff);
			return false;
		} else {
			tex_upload(h);
			texture->SetHandle(h);
			return true;
		}
	}
}

// set the given unit to reference the given texture; pass a null texture to disable texturing on any unit
void CRenderer::SetTexture(int unit,CTexture* texture)
{
	glActiveTexture(GL_TEXTURE0+unit);
	if (texture) {
		Handle h=texture->GetHandle();
		if (!h) {
			LoadTexture(texture);
			h=texture->GetHandle();
		} 

		// disable texturing if invalid handle
		if (h==0xffffffff) {
			glDisable(GL_TEXTURE_2D);
		} else {
			tex_bind(h);
			glEnable(GL_TEXTURE_2D);
		}
	} else {
		// switch off texturing on this unit
		glDisable(GL_TEXTURE_2D);
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

			if (MPatch->m_LastRenderedFrame == m_FrameCounter)
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

					glTexCoord2f (tu[0], tv[0]);

					if (g_HillShading)
						glColor3fv(&MPCurrent->m_pVertices[v1].m_Color.X);
					else
						glColor3f (1,1,1);

					glVertex3f (MPCurrent->m_pVertices[v1].m_Position.X,
								MPCurrent->m_pVertices[v1].m_Position.Y,
								MPCurrent->m_pVertices[v1].m_Position.Z);

					glTexCoord2f (tu[1], tv[1]);

					if (g_HillShading)
						glColor3fv(&MPCurrent->m_pVertices[v2].m_Color.X);
					else
						glColor3f (1,1,1);
					
					glVertex3f (MPCurrent->m_pVertices[v2].m_Position.X,
								MPCurrent->m_pVertices[v2].m_Position.Y,
								MPCurrent->m_pVertices[v2].m_Position.Z);

					tu[0]+=0.125f;
					tu[1]+=0.125f;
				}

				MPCurrent->m_LastRenderedFrame = m_FrameCounter;
				MPCurrent->m_RenderStage = RENDER_STAGE_BASE;

				if (!MPCurrent->m_pRightNeighbor)
					break;
				else
				{
					if (MPCurrent->m_pRightNeighbor->Tex1 != MPCurrent->Tex1 ||
						MPCurrent->m_pRightNeighbor->m_pParrent->m_LastVisFrame != m_FrameCounter)
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
	glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	for (int j=0; j<16; j++)
	{
		for (int i=0; i<16; i++)
		{
			MPatch = &(patch->m_MiniPatches[j][i]);

			if (MPatch->m_LastRenderedFrame == m_FrameCounter &&
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

						glMultiTexCoord2f (GL_TEXTURE0_ARB, tu[0], tv[0]);
						glMultiTexCoord2f (GL_TEXTURE1_ARB, tu[0]*2, tv[0]*2);
						
						if (g_HillShading)
							glColor3fv(&MPCurrent->m_pVertices[v1].m_Color.X);
						else
							glColor3f (1,1,1);

						glVertex3f (MPCurrent->m_pVertices[v1].m_Position.X,
									MPCurrent->m_pVertices[v1].m_Position.Y,
									MPCurrent->m_pVertices[v1].m_Position.Z);

						glMultiTexCoord2f (GL_TEXTURE0_ARB, tu[1], tv[1]);
						glMultiTexCoord2f (GL_TEXTURE1_ARB, tu[1]*2, tv[1]*2);
						
						if (g_HillShading)
							glColor3fv(&MPCurrent->m_pVertices[v1].m_Color.X);
						else
							glColor3f (1,1,1);
						
						glVertex3f (MPCurrent->m_pVertices[v2].m_Position.X,
									MPCurrent->m_pVertices[v2].m_Position.Y,
									MPCurrent->m_pVertices[v2].m_Position.Z);

						tu[0]+=0.125f;
						tu[1]+=0.125f;
					}

					MPCurrent->m_LastRenderedFrame = m_FrameCounter;
					MPCurrent->m_RenderStage = RENDER_STAGE_TRANS;

					if (!MPCurrent->m_pRightNeighbor)
						break;
					else
					{
						if (MPCurrent->m_pRightNeighbor->Tex2 != MPCurrent->Tex2 ||
						    MPCurrent->m_pRightNeighbor->m_AlphaMap != MPCurrent->m_AlphaMap ||
							MPCurrent->m_pRightNeighbor->m_pParrent->m_LastVisFrame != m_FrameCounter)
							break;
					}

					MPCurrent = MPCurrent->m_pRightNeighbor;
					start=1;
				}

				glEnd ();
			}
		}
	}
	glDisable (GL_BLEND);
	glActiveTexture (GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture (GL_TEXTURE0);
}
