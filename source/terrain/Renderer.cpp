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


#include <map>
#include <set>
#include <algorithm>
#include "Renderer.h"
#include "TransparencyRenderer.h"
#include "Terrain.h"
#include "Matrix3D.h"
#include "Camera.h"
#include "PatchRData.h"
#include "Texture.h"
#include "LightEnv.h"
#include "Visual.h"

#include "Model.h"
#include "ModelDef.h"

#include "types.h"
#include "ogl.h"
#include "res/mem.h"
#include "res/tex.h"


struct TGAHeader {
	// header stuff
	unsigned char  iif_size;            
	unsigned char  cmap_type;           
	unsigned char  image_type;          
	unsigned char  pad[5];

	// origin : unused
	unsigned short d_x_origin;
	unsigned short d_y_origin;
	
	// dimensions
	unsigned short width;
	unsigned short height;

	// bits per pixel : 16, 24 or 32
	unsigned char  bpp;          

	// image descriptor : Bits 3-0: size of alpha channel
	//					  Bit 4: must be 0 (reserved)
	//					  Bit 5: should be 0 (origin)
	//					  Bits 6-7: should be 0 (interleaving)
   unsigned char image_descriptor;    
};

static bool saveTGA(const char* filename,int width,int height,unsigned char* data) 
{
	FILE* fp=fopen(filename,"wb");
	if (!fp) return false;

	// fill file header
	TGAHeader header;
	header.iif_size=0;
	header.cmap_type=0;
	header.image_type=2;
	memset(header.pad,0,sizeof(header.pad));
	header.d_x_origin=0;
	header.d_y_origin=0;
	header.width=width;
	header.height=height;
	header.bpp=24;
	header.image_descriptor=0;

	if (fwrite(&header,sizeof(TGAHeader),1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// write data 
	if (fwrite(data,width*height*3,1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// return success ..
    fclose(fp);
	return true;
}

extern CTerrain g_Terrain;


CRenderer::CRenderer ()
{
	m_Width=0;
	m_Height=0;
	m_Depth=0;
	m_FrameCounter=0;
	m_TerrainRenderMode=SOLID;
	m_ModelRenderMode=SOLID;
}

CRenderer::~CRenderer ()
{
}

	
// EnumCaps: build card cap bits
void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_VBO=false;
#if 1
	// now start querying extensions
	if (oglExtAvail("GL_ARB_vertex_buffer_object")) {
		m_Caps.m_VBO=true;
	}
#endif
}

bool CRenderer::Open(int width, int height, int depth)
{
	m_Width = width;
	m_Height = height;
	m_Depth = depth;

	// set packing parameters
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);

	// setup default state
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f,0.0f,0.0f,0.0f);

	// query card capabilities
	EnumCaps();

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

	// zero out all the per-frame stats
	m_Stats.Reset();

	// calculate coefficients for terrain and unit lighting
	m_SHCoeffsUnits.Clear();
	m_SHCoeffsTerrain.Clear();

	if (m_LightEnv) {
		CVector3D dirlight;
		m_LightEnv->GetSunDirection(dirlight);
		m_SHCoeffsUnits.AddDirectionalLight(dirlight,m_LightEnv->m_SunColor);
		m_SHCoeffsTerrain.AddDirectionalLight(dirlight,m_LightEnv->m_SunColor);

		m_SHCoeffsUnits.AddAmbientLight(m_LightEnv->m_UnitsAmbientColor);
		m_SHCoeffsTerrain.AddAmbientLight(m_LightEnv->m_TerrainAmbientColor);		
	}

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CRenderer::RenderPatches()
{
	// switch on wireframe if we need it
	if (m_TerrainRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	} 

	// render all the patches, including blend pass
	RenderPatchSubmissions();
	
	if (m_TerrainRenderMode==WIREFRAME) {
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (m_TerrainRenderMode==EDGED_FACES) {
		// edged faces: need to make a second pass over the data:
		// first switch on wireframe
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		
		// setup some renderstate ..
		glDepthMask(0);
		SetTexture(0,0);
		glColor4f(1,1,1,0.35f);
		glLineWidth(2.0f);
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		// .. and some client states
		glEnableClientState(GL_VERTEX_ARRAY);

		uint i;

		// render each patch in wireframe
		for (i=0;i<m_TerrainPatches.size();++i) {
			CPatch* patch=m_TerrainPatches[i].m_Object;
			CPatchRData* patchdata=(CPatchRData*) patch->m_RenderData;
			patchdata->RenderWireframe();
		}

		// set color for outline
		glColor3f(0,0,1);
		glLineWidth(4.0f);
	
		// render outline of each patch 
		for (i=0;i<m_TerrainPatches.size();++i) {
			CPatch* patch=m_TerrainPatches[i].m_Object;
			CPatchRData* patchdata=(CPatchRData*) patch->m_RenderData;
			patchdata->RenderOutline();
		}

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
}

void CRenderer::RenderModelSubmissions()
{
	uint i;

	// first ensure all patches have up to date renderdata built for them; build up transparent passes
	// along the way
	for (i=0;i<m_Models.size();++i) {
		CVisual* visual=m_Models[i].m_Object;
		CModelRData* data=(CModelRData*) visual->m_Model->m_RenderData;
		if (data==0) {
			// no renderdata for model, create it now
			data=new CModelRData(visual->m_Model);
		} else {
			data->Update();
		}

		BuildTransparentPasses(visual);
	}

	// setup texture environment to modulate diffuse color with texture color
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// just pass through texture's alpha
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// setup client states
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render models
	for (i=0;i<m_Models.size();++i) {
		CVisual* visual=m_Models[i].m_Object;
		CModelRData* modeldata=(CModelRData*) visual->m_Model->m_RenderData;
		modeldata->Render(visual->GetTransform());
	}

	// switch off client states
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void CRenderer::RenderModels()
{
	// switch on wireframe if we need it
	if (m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	} 

	// render all the models
	RenderModelSubmissions();
	
	if (m_ModelRenderMode==WIREFRAME) {
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (m_ModelRenderMode==EDGED_FACES) {
		// edged faces: need to make a second pass over the data:
		// first switch on wireframe
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		
		// setup some renderstate ..
		glDepthMask(0);
		SetTexture(0,0);
		glColor4f(1,1,1,0.75f);
		glLineWidth(1.0f);
	
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

		// .. and some client states
		glEnableClientState(GL_VERTEX_ARRAY);

		// render each model
		for (uint i=0;i<m_Models.size();++i) {
			CVisual* visual=m_Models[i].m_Object;
			CModelRData* modeldata=(CModelRData*) visual->m_Model->m_RenderData;
			modeldata->RenderWireframe(visual->GetTransform());
		}

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
}

// force rendering of any batched objects
void CRenderer::FlushFrame()
{	
	// render submitted patches and models
	RenderPatches();

	RenderModels();

	// call on the transparency renderer to render all the transparent stuff
	g_TransparencyRenderer.Render();

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
	CMatrix3D view;
	camera.m_Orientation.Invert(view);
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

	m_Camera=camera;
}

void CRenderer::Submit(CPatch* patch)
{
	SSubmission<CPatch*> sub;
	sub.m_Object=patch;
	m_TerrainPatches.push_back(sub);
}

void CRenderer::Submit(CVisual* visual)
{
	SSubmission<CVisual*> sub;
	sub.m_Object=visual;
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

void CRenderer::RenderPatchSubmissions()
{
	uint i;
	// first ensure all patches have up to date renderdata built for them
	for (i=0;i<m_TerrainPatches.size();++i) {
		CPatch* patch=m_TerrainPatches[i].m_Object;
		CPatchRData* data=(CPatchRData*) patch->m_RenderData;
		if (data==0) {
			// no renderdata for patch, create it now
			data=new CPatchRData(patch);
		} else {
			data->Update();
		}
	}

	// set up client states for base pass
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// set up texture environment for base pass
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_ZERO);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);

	// render base passes for each patch
	for (i=0;i<m_TerrainPatches.size();++i) {
		CPatch* patch=m_TerrainPatches[i].m_Object;
		CPatchRData* patchdata=(CPatchRData*) patch->m_RenderData;
		patchdata->RenderBase();
	}

	// switch on the composite alpha map texture
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,m_CompositeAlphaMap);

	// setup additional texenv required by blend pass
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);

	// switch on blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// no need to write to the depth buffer a second time
	glDepthMask(0);

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render blend passes for each patch
	for (i=0;i<m_TerrainPatches.size();++i) {
		CPatch* patch=m_TerrainPatches[i].m_Object;
		CPatchRData* patchdata=(CPatchRData*) patch->m_RenderData;
		patchdata->RenderBlends();
	}

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	// restore depth writes
	glDepthMask(1);

	// restore default state: switch off blending
	glDisable(GL_BLEND);

	// switch off texture unit 1, make unit 0 active texture
	glActiveTexture(GL_TEXTURE1);
	glDisable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);

	// switch off all client states
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}



// try and load the given texture
bool CRenderer::LoadTexture(CTexture* texture)
{
	Handle h=texture->GetHandle();
	if (h) {
		// already tried to load this texture, nothing to do here - just return success according 
		// to whether this is a valid handle or not
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
void CRenderer::SetTexture(int unit,CTexture* texture,u32 wrapflags)
{
	glActiveTexture(GL_TEXTURE0+unit);
	if (texture) {
		Handle h=texture->GetHandle();
		if (!h) {
			LoadTexture(texture);
			h=texture->GetHandle();

			if (wrapflags) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapflags);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapflags);
			}
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

bool CRenderer::IsTextureTransparent(CTexture* texture)
{
	if (texture) {
		Handle h=texture->GetHandle();
		if (!h) {
			LoadTexture(texture);
			h=texture->GetHandle();
			if (h!=0xffffffff) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
			}
		} 
		if (h!=0xffffffff && h) {
			int fmt;
			int bpp;
			
			tex_info(h, NULL, NULL, &fmt, &bpp, NULL);
			if (bpp==24 || fmt == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
			{
				return false;
			}
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
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


inline void CopyTriple(unsigned char* dst,const unsigned char* src)
{
	dst[0]=src[0];
	dst[1]=src[1];
	dst[2]=src[2];
}

// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and 
// calculate the coordinate of each alphamap within this packed texture .. need to add
// validation that all maps are the same size
bool CRenderer::LoadAlphaMaps(const char* fnames[])
{
	glActiveTexture(GL_TEXTURE0_ARB);
	
	Handle textures[NumAlphaMaps];
	
	int i;

	for (i=0;i<NumAlphaMaps;i++) {
		textures[i]=tex_load(fnames[i]);
		if (textures[i] <= 0) {
			return false;
		}
	}

	int base;//=textures[0].width;
	
	i=tex_info(textures[0], &base, NULL, NULL, NULL, NULL);
	
	int size=(base+4)*NumAlphaMaps;
	int texsize=RoundUpToPowerOf2(size);
	
	unsigned char* data=new unsigned char[texsize*base*3];
	
	// for each tile on row
	for (i=0;i<NumAlphaMaps;i++) {
		//TEX& tex=textures[i];
		int bpp;
		// get src of copy
		const unsigned char* src;
		
		tex_info(textures[i], NULL, NULL, NULL, &bpp, (void **)&src);
		
		int srcstep=bpp/8;

		// get destination of copy
		unsigned char* dst=data+3*(i*(base+4));
		
		// for each row of image
		for (int j=0;j<base;j++) {
			// duplicate first pixel
			CopyTriple(dst,src);
			dst+=3;
			CopyTriple(dst,src);
			dst+=3;

			// copy a row
			for (int k=0;k<base;k++) {
				CopyTriple(dst,src);
				dst+=3;
				src+=srcstep;
			}
			// duplicate last pixel
			CopyTriple(dst,(src-bpp/8));
			dst+=3;
			CopyTriple(dst,(src-bpp/8));
			dst+=3;

			// advance write pointer for next row
			dst+=3*(texsize-(base+4));
		}

		m_AlphaMapCoords[i].u0=float(i*(base+4)+2)/float(texsize);
		m_AlphaMapCoords[i].u1=float((i+1)*(base+4)-2)/float(texsize);
		m_AlphaMapCoords[i].v0=0.0f;
		m_AlphaMapCoords[i].v1=1.0f;
	}

	glGenTextures(1,&m_CompositeAlphaMap);
	glBindTexture(GL_TEXTURE_2D,m_CompositeAlphaMap);
	glTexImage2D(GL_TEXTURE_2D,0,GL_INTENSITY,texsize,base,0,GL_RGB,GL_UNSIGNED_BYTE,data);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	delete[] data;
	return true;
}

void CRenderer::BuildTransparentPasses(CVisual* visual)
{
	if (!IsTextureTransparent(visual->m_Model->GetTexture())) {
		// ok, no transparency on this model .. ignore it here
		return;
	}

	// add this visual to the transparency renderer for later processing
	g_TransparencyRenderer.Add(visual);
}


