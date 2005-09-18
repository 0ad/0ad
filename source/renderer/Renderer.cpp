
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
#include "PlayerRenderer.h"
#include "Terrain.h"
#include "Matrix3D.h"
#include "Camera.h"
#include "PatchRData.h"
#include "Texture.h"
#include "LightEnv.h"
#include "Terrain.h"
#include "CLogger.h"
#include "ps/Game.h"
#include "Profile.h"

#include "Model.h"
#include "ModelDef.h"

#include "ogl.h"
#include "lib/res/file/file.h"
#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/ogl_tex.h"
#include "timer.h"

#define LOG_CATEGORY "graphics"

/*
// jw: unused
static bool saveTGA(const char* filename,int width,int height,int bpp,unsigned char* data)
{
	int err = tex_write(filename, width, height, bpp, TEX_BGR, data);
	return (err == 0);
}
*/

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

	m_RenderWater = true;
	m_WaterHeight = 5.0f;
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
		if (oglHaveExtension("GL_ARB_vertex_buffer_object")) {
			m_Caps.m_VBO=true;
		}
	}
	if (oglHaveExtension("GL_ARB_texture_border_clamp")) {
		m_Caps.m_TextureBorderClamp=true;
	}
	if (oglHaveExtension("GL_SGIS_generate_mipmap")) {
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

void CRenderer::SetOptionFloat(enum Option opt, float val)
{
	switch(opt)
	{
	case OPT_LODBIAS:
		m_Options.m_LodBias = val;
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
#ifndef SCED
	if(!g_Game || !g_Game->IsGameStarted())
		return;
#endif

	// bump frame counter
	m_FrameCounter++;

	// zero out all the per-frame stats
	m_Stats.Reset();

	// calculate coefficients for terrain and unit lighting
	m_SHCoeffsUnits.Clear();
	m_SHCoeffsTerrain.Clear();

	if (m_LightEnv) {
		m_SHCoeffsUnits.AddDirectionalLight(m_LightEnv->m_SunDir, m_LightEnv->m_SunColor);
		m_SHCoeffsTerrain.AddDirectionalLight(m_LightEnv->m_SunDir, m_LightEnv->m_SunColor);

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
	// ??? RC more optimal light placement?
	CVector3D lightpos=centre-(m_LightEnv->m_SunDir * 1000);

	// make light transformation matrix
	ConstructLightTransform(lightpos,m_LightEnv->m_SunDir,m_LightTransform);

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
	PROFILE( "render shadow map" );

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

	glActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, m_Options.m_LodBias);

	glColor4fv(m_Options.m_ShadowColor);

	glDisable(GL_CULL_FACE);

	// render models
	CModelRData::RenderModels(STREAM_POS,MODELFLAG_CASTSHADOWS);

	// call on the player renderer to render all of the player shadows.
	g_PlayerRenderer.RenderShadows();

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
	PROFILE( "applying shadows" );

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
	PROFILE(" render patches ");

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
		CPatchRData::RenderStreamsAll(STREAM_POS);

		// set color for outline
		glColor3f(0,0,1);
		glLineWidth(4.0f);

		// render outline of each patch
		CPatchRData::RenderOutlines();

		// .. and switch off the client states
		glDisableClientState(GL_VERTEX_ARRAY);

		// .. and restore the renderstates
		glDisable(GL_BLEND);
		glDepthMask(1);

		// restore fill mode, and we're done
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
}

void CRenderer::RenderWater()
{
	PROFILE(" render water ");

	if(!m_RenderWater) 
	{
		return;
	}

	const int DX[] = {1,1,0,0};
	const int DZ[] = {0,1,1,0};

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_TEXTURE_2D);
	glDepthMask(false);

	glBegin(GL_QUADS);

	for(size_t i=0; i<m_WaterPatches.size(); i++) 
	{
		CPatch* patch = m_WaterPatches[i];

		for(int dx=0; dx<PATCH_SIZE; dx++) 
		{
			for(int dz=0; dz<PATCH_SIZE; dz++) 
			{
				int x = (patch->m_X*PATCH_SIZE + dx) * CELL_SIZE;
				int z = (patch->m_Z*PATCH_SIZE + dz) * CELL_SIZE;

				// is any corner of the tile below the water height? if not, no point rendering it
				bool shouldRender = false;
				for(int j=0; j<4; j++) 
				{
					float vertX = x + DX[j]*CELL_SIZE;
					float vertZ = z + DZ[j]*CELL_SIZE;
					float terrainHeight = terrain->getExactGroundLevel(vertX, vertZ);
					if(terrainHeight < m_WaterHeight) 
					{
						shouldRender = true;
						break;
					}
				}
				if(!shouldRender) 
				{
					continue;
				}

				for(int j=0; j<4; j++) 
				{
					float vertX = x + DX[j]*CELL_SIZE;
					float vertZ = z + DZ[j]*CELL_SIZE;
					float terrainHeight = terrain->getExactGroundLevel(vertX, vertZ);
					float alpha = clamp((m_WaterHeight - terrainHeight) / 6.0f - 0.1f, -100.0f, 0.95f);
					glColor4f(0.1f, 0.3f, 0.8f, alpha);
					glVertex3f(vertX, m_WaterHeight, vertZ);
				}
			}
		}
	}

	glEnd();

	glDepthMask(true);
	glDisable(GL_BLEND);
}


void CRenderer::RenderModelSubmissions()
{
	// set up texture environment for base pass - modulate texture and primary color
	glActiveTextureARB(GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, m_Options.m_LodBias);

	// pass one through as alpha; transparent textures handled specially by CTransparencyRenderer
	// (gl_constant means the colour comes from the gl_texture_env_color)
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	float color[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);

	// setup client states
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// render models
	CModelRData::RenderModels(STREAM_POS|STREAM_COLOR|STREAM_UV0);

	// switch off client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void CRenderer::RenderModels()
{
	PROFILE( "render models ");

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
#ifndef SCED
	if(!g_Game || !g_Game->IsGameStarted())
		return;
#endif

	oglCheck();

	// sort all the transparent stuff
	g_TransparencyRenderer.Sort();

	if (!m_ShadowRendered) {
		if (m_Options.m_Shadows) {
			MICROLOG(L"render shadows");
			RenderShadowMap();
		}
		// clear buffers
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

	MICROLOG(L"render player models");
	g_PlayerRenderer.Render();
	oglCheck();

	// call on the transparency renderer to render all the transparent stuff
	MICROLOG(L"render transparent");
	g_TransparencyRenderer.Render();
	oglCheck();

	// render water (note: we're assuming there's no transparent stuff over water...
	// we could also do this above render transparent if we assume there's no transparent
	// stuff underwater)
	MICROLOG(L"render water");
	RenderWater();
	oglCheck();

	// empty lists
	MICROLOG(L"empty lists");
	g_TransparencyRenderer.Clear();
	g_PlayerRenderer.Clear();
	CPatchRData::ClearSubmissions();
	CModelRData::ClearSubmissions();
	m_WaterPatches.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// EndFrame: signal frame end; implicitly flushes batched objects
void CRenderer::EndFrame()
{
#ifndef SCED
	if(!g_Game || !g_Game->IsGameStarted())
		return;
#endif

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

void CRenderer::SubmitWater(CPatch* patch)
{
	m_WaterPatches.push_back(patch);
}

void CRenderer::Submit(CModel* model)
{
	if (model->GetFlags() & MODELFLAG_CASTSHADOWS) {
		PROFILE( "updating shadow bounds" );
		m_ShadowBound+=model->GetBounds();
	}

	CModelRData::Submit(model);
}

void CRenderer::Submit(CSprite* UNUSED(sprite))
{
}

void CRenderer::Submit(CParticleSys* UNUSED(psys))
{
}

void CRenderer::Submit(COverlay* UNUSED(overlay))
{
}

void CRenderer::RenderPatchSubmissions()
{
	// switch on required client states 
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	// render everything 
	CPatchRData::RenderBaseSplats();
	CPatchRData::RenderBlendSplats();

	// switch off all client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LoadTexture: try and load the given texture; set clamp/repeat flags on texture object if necessary
bool CRenderer::LoadTexture(CTexture* texture,u32 wrapflags)
{
	const Handle errorhandle = -1;

	Handle h=texture->GetHandle();
	if (h) {
		// already tried to load this texture, nothing to do here - just return success according
		// to whether this is a valid handle or not
		return h==errorhandle ? true : false;
	} else {
		h=ogl_tex_load(texture->GetName());
		if (h <= 0) {
			LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\"",(const char*) texture->GetName());
			texture->SetHandle(errorhandle);
			return false;
		} else {
			int tw=0,th=0;
			(void)ogl_tex_get_size(h, &tw, &th, 0);
			if(!is_pow2(tw) || !is_pow2(th)) {
				LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\" : not a power of 2 texture",(const char*) texture->GetName());
				ogl_tex_free(h);
				texture->SetHandle(errorhandle);
				return false;
			} else {
				ogl_tex_bind(h);
				ogl_tex_upload(h,GL_LINEAR_MIPMAP_LINEAR);

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
	glActiveTextureARB(GL_TEXTURE0+unit);
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

	glActiveTextureARB(GL_TEXTURE0+unit);

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
	Handle h = texture? texture->GetHandle() : 0;
	ogl_tex_bind(h, unit);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IsTextureTransparent: return true if given texture is transparent, else false - note texture must be loaded
// beforehand
bool CRenderer::IsTextureTransparent(CTexture* texture)
{
	if (!texture) return false;
	Handle h=texture->GetHandle();

	int flags = 0;	// assume no alpha on failure
	(void)ogl_tex_get_format(h, &flags, 0);
	return (flags & TEX_ALPHA) != 0;
}




inline void CopyTriple(unsigned char* dst,const unsigned char* src)
{
	dst[0]=src[0];
	dst[1]=src[1];
	dst[2]=src[2];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and
// calculate the coordinate of each alphamap within this packed texture
int CRenderer::LoadAlphaMaps()
{
	//
	// load all textures and store Handle in array
	//
	Handle textures[NumAlphaMaps];
	PathPackage pp;
	(void)pp_set_dir(&pp, "art/textures/terrain/alphamaps/special");
	const char* fnames[NumAlphaMaps] = {
		"blendcircle.png",
		"blendlshape.png",
		"blendedge.png",
		"blendedgecorner.png",
		"blendedgetwocorners.png",
		"blendfourcorners.png",
		"blendtwooppositecorners.png",
		"blendlshapecorner.png",
		"blendtwocorners.png",
		"blendcorner.png",
		"blendtwoedges.png",
		"blendthreecorners.png",
		"blendushape.png",
		"blendbad.png"
	};
	int base = 0;	// texture width/height (see below)
	// for convenience, we require all alpha maps to be of the same BPP
	// (avoids another ogl_tex_get_size call, and doesn't hurt)
	int bpp = 0;
	for(int i=0;i<NumAlphaMaps;i++)
	{
		(void)pp_append_file(&pp, fnames[i]);
		textures[i] = ogl_tex_load(pp.path);
		WARN_ERR(textures[i]);

		// get its size and make sure they are all equal.
		// (the packing algo assumes this)
		int this_width = 0, this_bpp = 0;	// fail-safe
		(void)ogl_tex_get_size(textures[i], &this_width, 0, &this_bpp);
		// .. first iteration: establish size
		if(i == 0)
		{
			base = this_width;
			bpp  = this_bpp;
		}
		// .. not first: make sure texture size matches
		else if(base != this_width || bpp != this_bpp)
			DISPLAY_ERROR(L"Alpha maps are not identically sized (including pixel depth)");
	}

	//
	// copy each alpha map (tile) into one buffer, arrayed horizontally.
	//
	int tile_w = 2+base+2;	// 2 pixel border (avoids bilinear filtering artifacts)
	int total_w = RoundUpToPowerOf2(tile_w * NumAlphaMaps);
	int total_h = base; debug_assert(is_pow2(total_h));
	u8* data=new u8[total_w*total_h*3];
	// for each tile on row
	for(int i=0;i<NumAlphaMaps;i++)
	{
		// get src of copy
		const u8* src = 0;
		(void)ogl_tex_get_data(textures[i], (void**)&src);

		int srcstep=bpp/8;

		// get destination of copy
		u8* dst=data+3*(i*tile_w);

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
			dst+=3*(total_w-tile_w);
		}

		m_AlphaMapCoords[i].u0=float(i*tile_w+2)/float(total_w);
		m_AlphaMapCoords[i].u1=float((i+1)*tile_w-2)/float(total_w);
		m_AlphaMapCoords[i].v0=0.0f;
		m_AlphaMapCoords[i].v1=1.0f;
	}

	for (int i=0;i<NumAlphaMaps;i++)
		ogl_tex_free(textures[i]);

	// upload the composite texture
	glGenTextures(1,(GLuint*) &m_CompositeAlphaMap);
	BindTexture(0,m_CompositeAlphaMap);
	glTexImage2D(GL_TEXTURE_2D,0,GL_INTENSITY,total_w,total_h,0,GL_RGB,GL_UNSIGNED_BYTE,data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	oglSquelchError(GL_INVALID_ENUM);	// GL_CLAMP_TO_EDGE
	delete[] data;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UnloadAlphaMaps: frees the resources allocates by LoadAlphaMaps
void CRenderer::UnloadAlphaMaps()
{
	glDeleteTextures(1, (GLuint*) &m_CompositeAlphaMap);
}
