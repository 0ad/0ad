
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


#include "precompiled.h"

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
#include "CLogger.h"
#include "ps/Game.h"

#include "Model.h"
#include "ModelDef.h"

#include "ogl.h"
#include "res/mem.h"
#include "res/ogl_tex.h"

#define LOG_CATEGORY "graphics"

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

static bool saveTGA(const char* filename,int width,int height,int bpp,unsigned char* data)
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
	header.width=(unsigned short)width;
	header.height=(unsigned short)height;
	header.bpp=(unsigned char)bpp;
	header.image_descriptor=(bpp==32) ? 8 : 0;

	if (fwrite(&header,sizeof(TGAHeader),1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// write data
	if (fwrite(data,width*height*bpp/8,1,fp)!=1) {
		fclose(fp);
		return false;
	}

	// return success ..
    fclose(fp);
	return true;
}


///////////////////////////////////////////////////////////////////////////////////
// CRenderer destructor
CRenderer::CRenderer()
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
	m_Options.m_Shadows=true;
	m_Options.m_ShadowColor=RGBAColor(0.4f,0.4f,0.4f,1.0f);

	for (uint i=0;i<MaxTextureUnits;i++) {
		m_ActiveTextures[i]=0;
	}
}

///////////////////////////////////////////////////////////////////////////////////
// CRenderer destructor
CRenderer::~CRenderer()
{
}


///////////////////////////////////////////////////////////////////////////////////
// EnumCaps: build card cap bits
void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_VBO=false;
	m_Caps.m_TextureBorderClamp=false;
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

	// query card capabilities
	EnumCaps();

	GLint bits;
	glGetIntegerv(GL_DEPTH_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: depth bits %d",bits);
	glGetIntegerv(GL_STENCIL_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: stencil bits %d",bits);
	glGetIntegerv(GL_ALPHA_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: alpha bits %d",bits);

	return true;
}

void CRenderer::Close()
{
}

// resize renderer view
void CRenderer::Resize(int width,int height)
{
	if (m_ShadowMap && (width>m_Width || height>m_Height)) {
		glDeleteTextures(1,(GLuint*) &m_ShadowMap);
		m_ShadowMap=0;
	}
	m_Width=width;
	m_Height=height;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionBool: set boolean renderer option
void CRenderer::SetOptionBool(enum Option opt,bool value)
{
	switch (opt) {
		case OPT_NOVBO:
			m_Options.m_NoVBO=value;
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
		case OPT_SHADOWS:
			return m_Options.m_Shadows;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
// SetOptionColor: set color renderer option
void CRenderer::SetOptionColor(enum Option opt,const RGBAColor& value)
{
	switch (opt) {
		case OPT_SHADOWCOLOR:
			m_Options.m_ShadowColor=value;
			break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// GetOptionColor: get color renderer option
const RGBAColor& CRenderer::GetOptionColor(enum Option opt) const
{
	static const RGBAColor defaultColor(1.0f,1.0f,1.0f,1.0f);

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
	if(!g_Game || !g_Game->IsGameStarted())
		return;

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
	m_ShadowBound.SetEmpty();
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
	const CBound& bounds=m_ShadowBound;

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
	znear=(znear<m_Camera.GetNearPlane()+0.01f) ? m_Camera.GetNearPlane() : znear-0.01f;
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

void CRenderer::CreateShadowMap()
{
	// get shadow map size as next power of two up from view width and height
	m_ShadowMapWidth=m_Width;
	m_ShadowMapWidth=RoundUpToPowerOf2(m_ShadowMapWidth);
	m_ShadowMapHeight=m_Height;
	m_ShadowMapHeight=RoundUpToPowerOf2(m_ShadowMapHeight);

	// create texture object - initially filled with white, so clamp to edge clamps to correct color
	glGenTextures(1,(GLuint*) &m_ShadowMap);
	BindTexture(0,(GLuint) m_ShadowMap);

	u32 size=m_ShadowMapWidth*m_ShadowMapHeight;
	u32* buf=new u32[size];
	for (uint i=0;i<size;i++) buf[i]=0x00ffffff;
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m_ShadowMapWidth,m_ShadowMapHeight,0,GL_RGBA,GL_UNSIGNED_BYTE,buf);
	delete[] buf;

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
}

void CRenderer::RenderShadowMap()
{
	// create shadow map if we haven't already got one
	if (!m_ShadowMap) CreateShadowMap();

	// clear buffers
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// build required matrices
	CalcShadowMatrices();

	// setup viewport
	glViewport(0,0,m_Width,m_Height);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(&m_LightProjection._11);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&m_LightTransform._11);

#if 0
	// debug aid - render actual bounds of shadow casting objects; helps see where
	// the lights projection/transform can be optimised
	glColor3f(1.0,0.0,0.0);
	const CBound& bounds=m_ShadowBound;

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
#endif // 0

	// setup client states
	glEnableClientState(GL_VERTEX_ARRAY);

	glEnable(GL_SCISSOR_TEST);
	glScissor(1,1,m_Width-2,m_Height-2);

	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glColor4fv(m_Options.m_ShadowColor);

	glDisable(GL_CULL_FACE);

	// render models
	CModelRData::RenderModels(STREAM_POS);

	// call on the transparency renderer to render all the transparent stuff
	g_TransparencyRenderer.RenderShadows();

	glEnable(GL_CULL_FACE);

	glColor3f(1.0f,1.0f,1.0f);

	glDisable(GL_SCISSOR_TEST);
	glDisableClientState(GL_VERTEX_ARRAY);

	// copy result into shadow map texture
	BindTexture(0,m_ShadowMap);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,m_Width,m_Height);

	// restore matrix stack
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

#if 0
	// debug aid - dump generated shadow map to file; helps verify shadow map
	// space being well used (not that it is at the minute .. (TODO, RC))
	unsigned char* data=new unsigned char[m_ShadowMapWidth*m_ShadowMapHeight*3];
	glGetTexImage(GL_TEXTURE_2D,0,GL_BGR_EXT,GL_UNSIGNED_BYTE,data);
	saveTGA("d:\\test4.tga",m_ShadowMapWidth,m_ShadowMapHeight,24,data);
	delete[] data;
#endif // 0
#if 0
	unsigned char* data=new unsigned char[m_Width*m_Height*4];
	glReadBuffer(GL_BACK);
	glReadPixels(0,0,m_Width,m_Height,GL_BGRA_EXT,GL_UNSIGNED_BYTE,data);
	saveTGA("d:\\test3.tga",m_Width,m_Height,32,data);
	delete[] data;
#endif // 0
}

void CRenderer::ApplyShadowMap()
{
	CMatrix3D tmp2;
	CMatrix3D texturematrix;

	float dx=0.5f*float(m_Width)/float(m_ShadowMapWidth);
	float dy=0.5f*float(m_Height)/float(m_ShadowMapHeight);
	texturematrix.SetTranslation(dx,dy,0);				// transform (-0.5, 0.5) to (0,1) - texture space
	tmp2.SetScaling(dx,dy,0);						// scale (-1,1) to (-0.5,0.5)
	texturematrix=texturematrix*tmp2;

	texturematrix=texturematrix*m_LightProjection;						// transform light -> projected light space (-1 to 1)
	texturematrix=texturematrix*m_LightTransform;						// transform world -> light space

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(&texturematrix._11);
	glMatrixMode(GL_MODELVIEW);

	CPatchRData::ApplyShadowMap(m_ShadowMap);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void CRenderer::RenderPatches()
{
	// switch on wireframe if we need it
	if (m_TerrainRenderMode==WIREFRAME) {
		MICROLOG(L"wireframe on");
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}

	// render all the patches, including blend pass
	MICROLOG(L"render patch submissions");
	RenderPatchSubmissions();

	if (m_TerrainRenderMode==WIREFRAME) {
		// switch wireframe off again
		MICROLOG(L"wireframe off");
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (m_TerrainRenderMode==EDGED_FACES) {
/*
		// TODO, RC - fix this
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
*/
	}
}


void CRenderer::RenderModelSubmissions()
{
	// set up texture environment for base pass - modulate texture and primary color
	glActiveTexture(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// pass one through as alpha; transparent textures handled specially by CTransparencyRenderer
	// (gl_constant means the colour comes from the environment (glColorxf))
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	// setup client states
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	// render models
	CModelRData::RenderModels(STREAM_POS|STREAM_COLOR|STREAM_UV0);

	// switch off client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
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
		CModelRData::RenderModels(STREAM_POS);

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
	// sort all the models by texture
//	std::sort(m_Models.begin(),m_Models.end(),SortModelsByTexture());

    if(!g_Game || !g_Game->IsGameStarted())
        return;

	oglCheck();

	// sort all the transparent stuff
	MICROLOG(L"sorting");
	g_TransparencyRenderer.Sort();
	if (!m_ShadowRendered) {
		if (m_Options.m_Shadows) {
			MICROLOG(L"render shadows");
			RenderShadowMap();
		}
		// clear buffers
		MICROLOG(L"clear buffer");
		glClearColor(m_ClearColor[0],m_ClearColor[1],m_ClearColor[2],m_ClearColor[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

	oglCheck();

	// render submitted patches and models
	MICROLOG(L"render patches");
	RenderPatches();
	oglCheck();

	MICROLOG(L"render models");
	RenderModels();
	oglCheck();

	if (m_Options.m_Shadows && !m_ShadowRendered) {
		MICROLOG(L"apply shadows");
		ApplyShadowMap();
		oglCheck();
	}
	m_ShadowRendered=true;

	// call on the transparency renderer to render all the transparent stuff
	MICROLOG(L"render transparent");
	g_TransparencyRenderer.Render();
	oglCheck();

	// empty lists
	MICROLOG(L"empty lists");
	g_TransparencyRenderer.Clear();
	CPatchRData::ClearSubmissions();
	CModelRData::ClearSubmissions();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EndFrame: signal frame end; implicitly flushes batched objects
void CRenderer::EndFrame()
{
    if(!g_Game || !g_Game->IsGameStarted())
        return;

	FlushFrame();
	g_Renderer.SetTexture(0,0);

	static bool once=false;
	if (!once && glGetError()) {
		LOG(ERROR, LOG_CATEGORY, "CRenderer::EndFrame: GL errors occurred");
		once=true;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// SetCamera: setup projection and transform of camera and adjust viewport to current view
void CRenderer::SetCamera(CCamera& camera)
{
	CMatrix3D view;
	camera.m_Orientation.GetInverse(view);
	const CMatrix3D& proj=camera.GetProjection();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&proj._11);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(&view._11);

	SetViewport(camera.GetViewPort());

	m_Camera=camera;
}

void CRenderer::SetViewport(const SViewPort &vp)
{
	glViewport(vp.m_X,vp.m_Y,vp.m_Width,vp.m_Height);
}

void CRenderer::Submit(CPatch* patch)
{
	CPatchRData::Submit(patch);
}

void CRenderer::Submit(CModel* model)
{
	if (1 /*ThisModelCastsShadows*/) {
		m_ShadowBound+=model->GetBounds();
	}

	CModelRData::Submit(model);
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
	// switch on required client states 
	MICROLOG(L"enable vertex array");
	glEnableClientState(GL_VERTEX_ARRAY);
	MICROLOG(L"enable color array");
	glEnableClientState(GL_COLOR_ARRAY);
	MICROLOG(L"enable tex coord array");
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// render everything 
	MICROLOG(L"render base splats");
	CPatchRData::RenderBaseSplats();
	MICROLOG(L"render blend splats");
	CPatchRData::RenderBlendSplats();

	// switch off all client states
	MICROLOG(L"disable tex coord array");
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	MICROLOG(L"disable color array");
	glDisableClientState(GL_COLOR_ARRAY);
	MICROLOG(L"disable vertex array");
	glDisableClientState(GL_VERTEX_ARRAY);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LoadTexture: try and load the given texture; set clamp/repeat flags on texture object if necessary
bool CRenderer::LoadTexture(CTexture* texture,u32 wrapflags)
{
	Handle h=texture->GetHandle();
	if (h) {
		// already tried to load this texture, nothing to do here - just return success according
		// to whether this is a valid handle or not
		return h==0xffffffff ? true : false;
	} else {
		h=tex_load(texture->GetName());
		if (h <= 0) {
			LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\"",(const char*) texture->GetName());
			texture->SetHandle(0xffffffff);
			return false;
		} else {
			int tw,th;
			tex_info(h, &tw, &th, NULL, NULL, NULL);

			tw&=(tw-1);
			th&=(th-1);
			if (tw || th) {
				texture->SetHandle(0xffffffff);
				LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\" : not a power of 2 texture",(const char*) texture->GetName());
				return false;
			} else {
				BindTexture(0,tex_id(h));
				tex_upload(h,GL_LINEAR_MIPMAP_LINEAR);

				if (wrapflags) {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapflags);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapflags);
				} else {
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				}
				texture->SetHandle(h);
				return true;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to current active unit
void CRenderer::BindTexture(int unit,GLuint tex)
{
#if 0
	glActiveTexture(GL_TEXTURE0+unit);
	if (tex==m_ActiveTextures[unit]) return;

	if (tex) {
		glBindTexture(GL_TEXTURE_2D,tex);
		if (!m_ActiveTextures[unit]) {
			glEnable(GL_TEXTURE_2D);
		}
	} else if (m_ActiveTextures[unit]) {
		glDisable(GL_TEXTURE_2D);
	}
	m_ActiveTextures[unit]=tex;
#endif

	glActiveTexture(GL_TEXTURE0+unit);

	glBindTexture(GL_TEXTURE_2D,tex);
	if (tex) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_TEXTURE_2D);
	}
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
