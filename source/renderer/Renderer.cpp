///////////////////////////////////////////////////////////////////////////////
//
// Name:		Renderer.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
// Description: OpenGL renderer class; a higher level interface
//	on top of OpenGL to handle rendering the basic visual games 
//	types - terrain, models, sprites, particles etc
//
///////////////////////////////////////////////////////////////////////////////


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

#include "Model.h"
#include "ModelDef.h"

#include "ogl.h"
#include "res/mem.h"
#include "res/tex.h"

#ifdef _WIN32
#include "pbuffer.h"
#endif

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
	m_ClearColor[0]=m_ClearColor[1]=m_ClearColor[2]=m_ClearColor[3]=0;
	m_ShadowMap=0;
	m_Options.m_NoVBO=false;
	m_Options.m_NoPBuffer=false;
	m_Options.m_Shadows=false;
	m_Options.m_ShadowColor=RGBColor(0.4f,0.4f,0.4f);
}

CRenderer::~CRenderer ()
{
}


///////////////////////////////////////////////////////////////////////////////////	
// EnumCaps: build card cap bits
void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_VBO=false;
	m_Caps.m_TextureBorderClamp=false;
	m_Caps.m_PBuffer=false;
	m_Caps.m_GenerateMipmaps=false;

	// now start querying extensions
	if (!m_Options.m_NoVBO) {
		if (oglExtAvail("GL_ARB_vertex_buffer_object")) {
			m_Caps.m_VBO=true;
		}
	}
	if (oglExtAvail("GL_ARB_texture_border_clamp")) {
		m_Caps.m_TextureBorderClamp=true;
	}
	if (oglExtAvail("GL_SGIS_generate_mipmap")) {
		m_Caps.m_GenerateMipmaps=true;
	}
	
	if (!m_Options.m_NoPBuffer) {
		extern bool PBufferQuery();
		if (PBufferQuery()) {
			m_Caps.m_PBuffer=true;
		}
	}
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
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0f,0.0f,0.0f,0.0f);

	// query card capabilities
	EnumCaps();

#ifdef _WIN32
	if (m_Caps.m_PBuffer) {
		// TODO, RC - query max pbuffer size
		PBufferInit(1024,1024,0,32,16,0);
	}
#endif
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
	if (m_ShadowMap) {
		glDeleteTextures(1,(GLuint*) &m_ShadowMap);
		m_ShadowMap=0;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionBool: set boolean renderer option 
void CRenderer::SetOptionBool(enum Option opt,bool value)
{
	switch (opt) {
		case OPT_NOVBO:
			m_Options.m_NoVBO=value;
			break;
		case OPT_NOPBUFFER:
			m_Options.m_NoPBuffer=value;
			break;
		case OPT_SHADOWS:
			m_Options.m_Shadows=value;
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// GetOptionBool: get boolean renderer option 
bool CRenderer::GetOptionBool(enum Option opt) const
{
	switch (opt) {
		case OPT_NOVBO:
			return m_Options.m_NoVBO;
		case OPT_NOPBUFFER:
			return m_Options.m_NoPBuffer;
		case OPT_SHADOWS:
			return m_Options.m_Shadows;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionColor: set color renderer option 
void CRenderer::SetOptionColor(enum Option opt,const RGBColor& value)
{
	switch (opt) {
		case OPT_SHADOWCOLOR:
			m_Options.m_ShadowColor=value;
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// GetOptionColor: get color renderer option 
const RGBColor& CRenderer::GetOptionColor(enum Option opt) const
{
	static const RGBColor defaultColor(1.0f,1.0f,1.0f);

	switch (opt) {
		case OPT_SHADOWCOLOR:
			return m_Options.m_ShadowColor;
	}

	return defaultColor;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BeginFrame: signal frame start
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

	// init per frame stuff
	m_ShadowRendered=false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetClearColor: set color used to clear screen in BeginFrame()
void CRenderer::SetClearColor(u32 color)
{ 
	m_ClearColor[0]=float(color & 0xff)/255.0f;
	m_ClearColor[1]=float((color>>8) & 0xff)/255.0f;
	m_ClearColor[2]=float((color>>16) & 0xff)/255.0f;
	m_ClearColor[3]=float((color>>24) & 0xff)/255.0f;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildTransformation: build transformation matrix from a position and standard basis vectors
void CRenderer::BuildTransformation(const CVector3D& pos,const CVector3D& right,const CVector3D& up,
						 const CVector3D& dir,CMatrix3D& result)
{
	// build basis
	result._11=right.X;
	result._12=right.Y;
	result._13=right.Z;
	result._14=0;

	result._21=up.X;
	result._22=up.Y;
	result._23=up.Z;
	result._24=0;

	result._31=dir.X;
	result._32=dir.Y;
	result._33=dir.Z;
	result._34=0;

	result._41=0;
	result._42=0;
	result._43=0;
	result._44=1;

	CMatrix3D trans;
	trans.SetTranslation(-pos.X,-pos.Y,-pos.Z);
	result=result*trans;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// ConstructLightTransform: build transformation matrix for light at given position casting in 
// given direction
void CRenderer::ConstructLightTransform(const CVector3D& pos,const CVector3D& dir,CMatrix3D& result)
{
	CVector3D right,up;

	CVector3D viewdir=m_Camera.m_Orientation.GetIn();
	if (fabs(dir.Y)>0.01f) {
		up=CVector3D(viewdir.X,(-dir.Z*viewdir.Z-dir.X*dir.X)/dir.Y,viewdir.Z);
	} else {
		up=CVector3D(0,0,1);
	}

	up.Normalize();
	right=dir.Cross(up);
	right.Normalize();
	BuildTransformation(pos,right,up,dir,result);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CalcShadowMatrices: calculate required matrices for shadow map generation - the light's 
// projection and transformation matrices
void CRenderer::CalcShadowMatrices()
{
	int i;

	// get bounds of shadow casting objects
	CBound bounds;
	CalcShadowBounds(bounds);

	// get centre of bounds
	CVector3D centre;
	bounds.GetCentre(centre);

	// get sunlight direction
	CVector3D lightdir;
	// ??? RC using matrix rotation to get sun direction?
	m_LightEnv->GetSunDirection(lightdir);
	// ??? RC more optimal light placement?
	CVector3D lightpos=centre-(lightdir*1000);

	// make light transformation matrix
	ConstructLightTransform(lightpos,lightdir,m_LightTransform);	
	
	// transform shadow bounds to light space, calculate near and far bounds
	CVector3D vp[8];
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[0].Z),vp[0]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[0].Z),vp[1]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[0].Z),vp[2]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[0].Z),vp[3]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[1].Z),vp[4]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[1].Z),vp[5]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[1].Z),vp[6]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[1].Z),vp[7]);

	float left=vp[0].X;
	float right=vp[0].X;
	float top=vp[0].Y;
	float bottom=vp[0].Y;
	float znear=vp[0].Z;
	float zfar=vp[0].Z;

	for (i=1;i<8;i++) {
		if (vp[i].X<left) left=vp[i].X;
		else if (vp[i].X>right) right=vp[i].X;

		if (vp[i].Y<bottom) bottom=vp[i].Y;
		else if (vp[i].Y>top) top=vp[i].Y;

		if (vp[i].Z<znear) znear=vp[i].Z;
		else if (vp[i].Z>zfar) zfar=vp[i].Z;
	}

	// shift near and far clip planes slightly to avoid artifacts with points
	// exactly on the clip planes
	znear-=0.01f;
	zfar+=0.01f;

	m_LightProjection.SetZero();
	m_LightProjection._11=2/(right-left);
	m_LightProjection._22=2/(top-bottom);
	m_LightProjection._33=2/(zfar-znear);
	m_LightProjection._14=-(right+left)/(right-left);
	m_LightProjection._24=-(top+bottom)/(top-bottom);
	m_LightProjection._34=-(zfar+znear)/(zfar-znear);	
	m_LightProjection._44=1;

#if 0

#if 0
	// TODO, RC - trim against frustum?
	// get points of view frustum in world space
	CVector3D frustumPts[8];
	m_Camera.GetFrustumPoints(frustumPts);

	// transform to light space
	for (i=0;i<8;i++) {
		m_LightTransform.Transform(frustumPts[i],vp[i]);
	}
	
	float left1=vp[0].X;
	float right1=vp[0].X;
	float top1=vp[0].Y;
	float bottom1=vp[0].Y;
	float znear1=vp[0].Z;
	float zfar1=vp[0].Z;

	for (int i=1;i<8;i++) {
		if (vp[i].X<left1) left1=vp[i].X;
		else if (vp[i].X>right1) right1=vp[i].X;

		if (vp[i].Y<bottom1) bottom1=vp[i].Y;
		else if (vp[i].Y>top1) top1=vp[i].Y;

		if (vp[i].Z<znear1) znear1=vp[i].Z;
		else if (vp[i].Z>zfar1) zfar1=vp[i].Z;
	}

	left=max(left,left1);
	right=min(right,right1);
	top=min(top,top1);
	bottom=max(bottom,bottom1);
	znear=max(znear,znear1);
	zfar=min(zfar,zfar1);
#endif

	// experimental stuff, do not use ..
	// TODO, RC - desperately need to improve resolution here if we're using shadow maps; investigate
	// feasibility of PSMs
	
	// transform light space bounds to image space - TODO, RC: safe to just use 3d transform here?
	CVector4D vph[8];
	for (i=0;i<8;i++) {
		CVector4D tmp(vp[i].X,vp[i].Y,vp[i].Z,1.0f);
		m_LightProjection.Transform(tmp,vph[i]);
		vph[i][0]/=vph[i][2];
		vph[i][1]/=vph[i][2];
	}

	// find the two points furthest apart
	int p0,p1;
	float maxdistsqrd=-1;
	for (i=0;i<8;i++) {
		for (int j=i+1;j<8;j++) {
			float dx=vph[i][0]-vph[j][0];
			float dy=vph[i][1]-vph[j][1];
			float distsqrd=dx*dx+dy*dy;
			if (distsqrd>maxdistsqrd) {
				p0=i;
				p1=j;
				maxdistsqrd=distsqrd;
			}
		}
	}

	// now we want to rotate the camera such that the longest axis lies the diagonal at 45 degrees - 
	// get angle between points
	float angle=atan2(vph[p0][1]-vph[p1][1],vph[p0][0]-vph[p1][0]);
	float rotation=-angle;

	// build rotation matrix
	CQuaternion quat;
	quat.FromAxisAngle(lightdir,rotation);
	CMatrix3D m;
	quat.ToMatrix(m);

	// rotate up vector by given rotation
	CVector3D up(m_LightTransform._21,m_LightTransform._22,m_LightTransform._23);
	up=m.Rotate(up);
	up.Normalize();		// TODO, RC - required??
	
	// rebuild right vector
	CVector3D rightvec;
	rightvec=lightdir.Cross(up);
	rightvec.Normalize();
	BuildTransformation(lightpos,rightvec,up,lightdir,m_LightTransform);

	// retransform points
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[0].Z),vp[0]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[0].Z),vp[1]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[0].Z),vp[2]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[0].Z),vp[3]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[1].Z),vp[4]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[1].Z),vp[5]);
	m_LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[1].Z),vp[6]);
	m_LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[1].Z),vp[7]);

	// recalculate projection
	left=vp[0].X;
	right=vp[0].X;
	top=vp[0].Y;
	bottom=vp[0].Y;
	znear=vp[0].Z;
	zfar=vp[0].Z;

	for (i=1;i<8;i++) {
		if (vp[i].X<left) left=vp[i].X;
		else if (vp[i].X>right) right=vp[i].X;

		if (vp[i].Y<bottom) bottom=vp[i].Y;
		else if (vp[i].Y>top) top=vp[i].Y;

		if (vp[i].Z<znear) znear=vp[i].Z;
		else if (vp[i].Z>zfar) zfar=vp[i].Z;
	}

	// shift near and far clip planes slightly to avoid artifacts with points
	// exactly on the clip planes
	znear-=0.01f;
	zfar+=0.01f;

	m_LightProjection.SetZero();
	m_LightProjection._11=2/(right-left);
	m_LightProjection._22=2/(top-bottom);
	m_LightProjection._33=2/(zfar-znear);
	m_LightProjection._14=-(right+left)/(right-left);
	m_LightProjection._24=-(top+bottom)/(top-bottom);
	m_LightProjection._34=-(zfar+znear)/(zfar-znear);	
	m_LightProjection._44=1;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// CalcShadowBounds: calculate the bounding box encompassing all visible shadow casting
// objects
void CRenderer::CalcShadowBounds(CBound& bounds)
{
	bounds.SetEmpty();

	for (uint i=0;i<m_Models.size();++i) {
		CModel* model=m_Models[i];
		bounds+=model->GetBounds();
	}
}

void CRenderer::CreateShadowMap()
{
	// get shadow map size as next power of two down from minimum of view width and height
	if (!m_Caps.m_PBuffer) {
		m_ShadowMapSize=m_Width<m_Height ? m_Width : m_Height;
		m_ShadowMapSize=RoundUpToPowerOf2(m_ShadowMapSize);
		m_ShadowMapSize>>=1;
	} else {
		m_ShadowMapSize=PBufferWidth();
	}

	// create texture object
	glGenTextures(1,(GLuint*) &m_ShadowMap);
	BindTexture(0,(GLuint) m_ShadowMap);
	glTexImage2D(GL_TEXTURE_2D,0,m_Depth==16 ? GL_RGBA4 : GL_RGBA8,m_ShadowMapSize,m_ShadowMapSize,0,GL_RGBA,GL_UNSIGNED_BYTE,0);

	// set texture parameters
	if (m_Caps.m_TextureBorderClamp) {
		const float borderColour[4]={1,1,1,0};
		glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,borderColour);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	} else {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

void CRenderer::RenderShadowMap()
{
	// create shadow map if we haven't already got one
	if (!m_ShadowMap) CreateShadowMap();

	if (m_Caps.m_PBuffer) {
		PBufferMakeCurrent();
	}

	// clear buffers
	glClearColor(1,1,1,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// build required matrices
	CalcShadowMatrices();

	// setup viewport
	glViewport(0,0,m_ShadowMapSize,m_ShadowMapSize);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(&m_LightProjection._11);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&m_LightTransform._11);

	if (0)
	{
		// debug aid - render actual bounds of shadow casting objects; helps see where 
		// the lights projection/transform can be optimised
		glColor3f(1.0,0.0,0.0);
		CBound bounds;
		CalcShadowBounds(bounds);

		glBegin(GL_LINE_LOOP);
		glVertex3f(bounds[0].X,bounds[0].Y,bounds[0].Z);
		glVertex3f(bounds[0].X,bounds[0].Y,bounds[1].Z);
		glVertex3f(bounds[0].X,bounds[1].Y,bounds[1].Z);
		glVertex3f(bounds[0].X,bounds[1].Y,bounds[0].Z);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(bounds[1].X,bounds[0].Y,bounds[0].Z);
		glVertex3f(bounds[1].X,bounds[0].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[1].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[1].Y,bounds[0].Z);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(bounds[0].X,bounds[0].Y,bounds[0].Z);
		glVertex3f(bounds[0].X,bounds[0].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[0].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[0].Y,bounds[0].Z);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(bounds[0].X,bounds[1].Y,bounds[0].Z);
		glVertex3f(bounds[0].X,bounds[1].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[1].Y,bounds[1].Z);
		glVertex3f(bounds[1].X,bounds[1].Y,bounds[0].Z);
		glEnd();
	}

	// setup client states
	glEnableClientState(GL_VERTEX_ARRAY);

	if (!m_Caps.m_TextureBorderClamp) {
		glEnable(GL_SCISSOR_TEST);
		glScissor(1,1,m_ShadowMapSize-2,m_ShadowMapSize-2);
	}

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_ONE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glColor3fv(&m_Options.m_ShadowColor.X);

	// render models
	RenderModelsStreams(STREAM_POS);

	// call on the transparency renderer to render all the transparent stuff
	g_TransparencyRenderer.RenderShadows();

	glColor3f(1.0f,1.0f,1.0f);

	if (!m_Caps.m_TextureBorderClamp) {
		glDisable(GL_SCISSOR_TEST);
	}
	glDisableClientState(GL_VERTEX_ARRAY);

	// copy result into shadow map texture
	BindTexture(0,m_ShadowMap);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,m_ShadowMapSize,m_ShadowMapSize);

	// restore matrix stack
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	if (0)
	{	
		// debug aid - dump generated shadow map to file; helps verify shadow map
		// space being well used (no guarantee this'll work on pbuffer - may have a single buffered one)
		unsigned char* data=new unsigned char[m_ShadowMapSize*m_ShadowMapSize*3];
		glReadBuffer(GL_BACK);
		glReadPixels(0,0,m_ShadowMapSize,m_ShadowMapSize,GL_BGR_EXT,GL_UNSIGNED_BYTE,data);	
		saveTGA("d:\\test.tga",m_ShadowMapSize,m_ShadowMapSize,data);
		delete[] data;
	}
	
	if (m_Caps.m_PBuffer) {
		PBufferMakeUncurrent();
	}

	// restore viewport
	const SViewPort& vp=m_Camera.GetViewPort();
	glViewport(vp.m_X, vp.m_Y, vp.m_Width, vp.m_Height);
}

void CRenderer::ApplyShadowMap()
{
	BindTexture(0,m_ShadowMap);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_ONE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	glColor3f(1,1,1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR,GL_ZERO);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	CMatrix3D tmp2;
	CMatrix3D texturematrix;

	texturematrix.SetTranslation(0.5f,0.5f,0.5f);				// transform (-0.5, 0.5) to (0,1) - texture space
	tmp2.SetScaling(0.5f,0.5f,0.5f);						// scale (-1,1) to (-0.5,0.5)
	texturematrix=texturematrix*tmp2;
	
	texturematrix=texturematrix*m_LightProjection;						// transform light -> projected light space (-1 to 1)
	texturematrix=texturematrix*m_LightTransform;						// transform world -> light space

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(&texturematrix._11);
	glMatrixMode(GL_MODELVIEW);

	for (uint i=0;i<m_TerrainPatches.size();++i) {
		CPatch* patch=m_TerrainPatches[i];
		CPatchRData* patchdata=(CPatchRData*) patch->GetRenderData();
		patchdata->RenderStreams(STREAM_POS|STREAM_POSTOUV0);
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);	
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
			CPatch* patch=m_TerrainPatches[i];
			CPatchRData* patchdata=(CPatchRData*) patch->GetRenderData();
			patchdata->RenderStreams(STREAM_POS);
		}

		// set color for outline
		glColor3f(0,0,1);
		glLineWidth(4.0f);
	
		// render outline of each patch 
		for (i=0;i<m_TerrainPatches.size();++i) {
			CPatch* patch=m_TerrainPatches[i];
			CPatchRData* patchdata=(CPatchRData*) patch->GetRenderData();
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

//////////////////////////////////////////////////////////////////////////////////////////
// RenderModelsStreams: recurse down given model rendering given streams on each model						   
void CRenderer::RenderModelsStreams(u32 streamflags)
{
	for (uint i=0;i<m_Models.size();++i) {
		CModel* model=m_Models[i];
		// push transform onto stack
		glPushMatrix();
		glMultMatrixf(&model->GetTransform()._11);	

		// render this model
		CModelRData* modeldata=(CModelRData*) model->GetRenderData();
		modeldata->RenderStreams(streamflags);

		// pop transform off stack
		glPopMatrix();
	}
}

void CRenderer::RenderModelSubmissions()
{
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
	RenderModelsStreams(STREAM_POS|STREAM_COLOR|STREAM_UV0);

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
		RenderModelsStreams(STREAM_POS);

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SortModelsByTexture: sorting class used for batching models with identical textures
struct SortModelsByTexture {
	typedef CModel* SortObj;
	
	bool operator()(const SortObj& lhs,const SortObj& rhs) {
		return lhs->GetTexture()<rhs->GetTexture() ? true : false;
	}
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// FlushFrame: force rendering of any batched objects
void CRenderer::FlushFrame()
{	
	// update renderdata of everything submitted
	UpdateSubmittedObjectData();

	// sort all the models by texture
	std::sort(m_Models.begin(),m_Models.end(),SortModelsByTexture());

	// sort all the transparent stuff
	g_TransparencyRenderer.Sort();

	if (!m_ShadowRendered) {
		if (m_Models.size()>0 && m_Options.m_Shadows) RenderShadowMap();
		m_ShadowRendered=true;
		// clear buffers
		glClearColor(m_ClearColor[0],m_ClearColor[1],m_ClearColor[2],m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// render submitted patches and models
	RenderPatches();
	if (m_Models.size()>0) {
		if (m_Options.m_Shadows) ApplyShadowMap();
		RenderModels();
		// call on the transparency renderer to render all the transparent stuff
		g_TransparencyRenderer.Render();
	}
	
	// empty lists
	g_TransparencyRenderer.Clear();
	m_TerrainPatches.clear();
	m_Models.clear();
}

// signal frame end : implicitly flushes batched objects 
void CRenderer::EndFrame()
{
	FlushFrame();
	g_Renderer.SetTexture(0,0);
}

void CRenderer::SetCamera(CCamera& camera)
{
	CMatrix3D view;
	camera.m_Orientation.GetInverse(view);
	CMatrix3D proj = camera.GetProjection();

	float gl_view[16] = {view._11, view._21, view._31, view._41,
						 view._12, view._22, view._32, view._42,
						 view._13, view._23, view._33, view._43,
						 view._14, view._24, view._34, view._44};

	float gl_proj[16] = {proj._11, proj._21, proj._31, proj._41,
						 proj._12, proj._22, proj._32, proj._42,
						 proj._13, proj._23, proj._33, proj._43,
						 proj._14, proj._24, proj._34, proj._44};


	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(gl_proj);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(gl_view);

	const SViewPort& vp = camera.GetViewPort();
	glViewport (vp.m_X, vp.m_Y, vp.m_Width, vp.m_Height);

	m_Camera=camera;
}

void CRenderer::Submit(CPatch* patch)
{
	m_TerrainPatches.push_back(patch);
}

void CRenderer::Submit(CModel* model)
{
	m_Models.push_back(model);
}

void CRenderer::Submit(CSprite* sprite)
{
}

void CRenderer::Submit(CParticleSys* psys)
{
}

void CRenderer::Submit(COverlay* overlay)
{
}

void CRenderer::RenderPatchSubmissions()
{
	uint i;

	// set up client states for base pass
	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// set up texture environment for base pass
	glActiveTexture(GL_TEXTURE0);
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
		CPatch* patch=m_TerrainPatches[i];
		CPatchRData* patchdata=(CPatchRData*) patch->GetRenderData();
		patchdata->RenderBase();
	}

	// switch on the composite alpha map texture
	BindTexture(1,m_CompositeAlphaMap);

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
		CPatch* patch=m_TerrainPatches[i];
		CPatchRData* patchdata=(CPatchRData*) patch->GetRenderData();
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



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LoadTexture: try and load the given texture; set clamp/repeat flags on texture object if necessary
bool CRenderer::LoadTexture(CTexture* texture,u32 wrapflags)
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

			BindTexture(0,tex_id(h));
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			if (wrapflags) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapflags);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapflags);
			}

			texture->SetHandle(h);
			return true;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to given unit
void CRenderer::BindTexture(int unit,GLuint tex)
{
	glActiveTexture(GL_TEXTURE0+unit);
//	if (tex==m_ActiveTextures[unit]) return;
	
	if (tex) {
		glEnable(GL_TEXTURE_2D);	
	} else {
		glDisable(GL_TEXTURE_2D);
	}
	glBindTexture(GL_TEXTURE_2D,tex);
	m_ActiveTextures[unit]=tex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTexture: set the given unit to reference the given texture; pass a null texture to disable texturing on any unit
void CRenderer::SetTexture(int unit,CTexture* texture)
{
	if (texture) {
		Handle h=texture->GetHandle();
		BindTexture(unit,tex_id(h));
	} else {
		BindTexture(unit,0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsTextureTransparent: return true if given texture is transparent, else false - note texture must be loaded
// beforehand
bool CRenderer::IsTextureTransparent(CTexture* texture)
{
	if (!texture) return false;
	
	Handle h=texture->GetHandle();
	if (h<=0) return false;
		
	int fmt;
	int bpp;
	
	tex_info(h, NULL, NULL, &fmt, &bpp, NULL);
	if (bpp==24 || fmt == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) {
		return false;
	}
	return true;
}




inline void CopyTriple(unsigned char* dst,const unsigned char* src)
{
	dst[0]=src[0];
	dst[1]=src[1];
	dst[2]=src[2];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
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

	int base;
	
	i=tex_info(textures[0], &base, NULL, NULL, NULL, NULL);
	
	int size=(base+4)*NumAlphaMaps;
	int texsize=RoundUpToPowerOf2(size);

	unsigned char* data=new unsigned char[texsize*base*3];
	
	// for each tile on row
	for (i=0;i<NumAlphaMaps;i++) {

		int bpp;
		// get src of copy
		const u8* src;
		
		tex_info(textures[i], NULL, NULL, NULL, &bpp, (void **)&src);
		
		int srcstep=bpp/8;

		// get destination of copy
		u8* dst=data+3*(i*(base+4));
		
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
			CopyTriple(dst,(src-srcstep));
			dst+=3;
			CopyTriple(dst,(src-srcstep));
			dst+=3;

			// advance write pointer for next row
			dst+=3*(texsize-(base+4));
		}

		m_AlphaMapCoords[i].u0=float(i*(base+4)+2)/float(texsize);
		m_AlphaMapCoords[i].u1=float((i+1)*(base+4)-2)/float(texsize);
		m_AlphaMapCoords[i].v0=0.0f;
		m_AlphaMapCoords[i].v1=1.0f;
	}

	glGenTextures(1,(GLuint*) &m_CompositeAlphaMap);
	BindTexture(0,m_CompositeAlphaMap);
	glTexImage2D(GL_TEXTURE_2D,0,GL_INTENSITY,texsize,base,0,GL_RGB,GL_UNSIGNED_BYTE,data);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	delete[] data;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildTransparentPasses: add deferred passes from given model to the transparency renderer
void CRenderer::BuildTransparentPasses(CModel* model)
{
	CModelRData* data=(CModelRData*) model->GetRenderData();
	assert(data);
	if (data->GetFlags() & MODELRDATA_FLAG_TRANSPARENT) {
		// add this mode to the transparency renderer for later processing - calculate 
		// transform matrix
		g_TransparencyRenderer.Add(model);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateModelDataRecursive: recurse down given model building renderdata for it
// and all it's children
void CRenderer::UpdateModelDataRecursive(CModel* model)
{
	CModelRData* data=(CModelRData*) model->GetRenderData();
	if (data==0) {
		// no renderdata for model, create it now
		data=new CModelRData(model);
		model->SetRenderData(data);
	} else {
		data->Update();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateSubmittedObjectData: ensure all submitted objects have renderdata and that it is up to date 
// - call once before doing anything with any objects
void CRenderer::UpdateSubmittedObjectData()
{
	uint i;

	// ensure all patches have up to date renderdata built for them
	for (i=0;i<m_TerrainPatches.size();++i) {
		CPatch* patch=m_TerrainPatches[i];
		CPatchRData* data=(CPatchRData*) patch->GetRenderData();
		if (data==0) {
			// no renderdata for patch, create it now
			data=new CPatchRData(patch);
			patch->SetRenderData(data);
		} else {
			data->Update();
		}
	}

	// ensure all models have up to date renderdata built for them
	for (i=0;i<m_Models.size();++i) {
		UpdateModelDataRecursive(m_Models[i]);
		// (recursively) build transparent passes from model
		BuildTransparentPasses(m_Models[i]);
	}
}

