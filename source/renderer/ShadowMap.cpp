/**
 * =========================================================================
 * File        : ShadowMap.cpp
 * Project     : Pyrogenesis
 * Description : Shadow mapping related texture and matrix management
 *
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#include "precompiled.h"

#include "ogl.h"

#include "graphics/LightEnv.h"

#include "maths/Bound.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"

#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
// ShadowMap implementation

/**
 * Struct ShadowMapInternals: Internal data for the ShadowMap implementation
 */
struct ShadowMapInternals
{
	// handle of shadow map
	GLuint Texture;
	// width, height of shadow map
	u32 Width, Height;
	// object space bound of shadow casting objects
	CBound m_ShadowBound;
	// project light space into projected light space
	CMatrix3D LightProjection;
	// transform world space into light space
	CMatrix3D LightTransform;
	// transform world space into texture space
	CMatrix3D TextureMatrix;
	
	// Helper functions
	void BuildTransformation(
			const CVector3D& pos,const CVector3D& right,const CVector3D& up,
			const CVector3D& dir,CMatrix3D& result);
	void ConstructLightTransform(const CVector3D& pos,const CVector3D& dir,CMatrix3D& result);
	void CalcShadowMatrices(const CBound& bound);
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
ShadowMap::ShadowMap()
{
	m = new ShadowMapInternals;
	m->Texture = 0;
}


ShadowMap::~ShadowMap()
{
	if (m->Texture)
		glDeleteTextures(1, &m->Texture);

	delete m;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildTransformation: build transformation matrix from a position and standard basis vectors
// TODO: Shouldn't this be part of CMatrix3D?
void ShadowMapInternals::BuildTransformation(
		const CVector3D& pos,const CVector3D& right,const CVector3D& up,
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
void ShadowMapInternals::ConstructLightTransform(const CVector3D& pos,const CVector3D& dir,CMatrix3D& result)
{
	CVector3D right,up;

	CVector3D viewdir = g_Renderer.GetCamera().m_Orientation.GetIn();
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
void ShadowMapInternals::CalcShadowMatrices(const CBound& bounds)
{
	const CLightEnv& lightenv = g_Renderer.GetLightEnv();
	const CCamera& camera = g_Renderer.GetCamera();
	int i;

	// get centre of bounds
	CVector3D centre;
	bounds.GetCentre(centre);

	// get sunlight direction
	// ??? RC more optimal light placement?
	CVector3D lightpos=centre-(lightenv.m_SunDir * 1000);

	// make light transformation matrix
	ConstructLightTransform(lightpos, lightenv.m_SunDir, LightTransform);

	// transform shadow bounds to light space, calculate near and far bounds
	CVector3D vp[8];
	LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[0].Z),vp[0]);
	LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[0].Z),vp[1]);
	LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[0].Z),vp[2]);
	LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[0].Z),vp[3]);
	LightTransform.Transform(CVector3D(bounds[0].X,bounds[0].Y,bounds[1].Z),vp[4]);
	LightTransform.Transform(CVector3D(bounds[1].X,bounds[0].Y,bounds[1].Z),vp[5]);
	LightTransform.Transform(CVector3D(bounds[0].X,bounds[1].Y,bounds[1].Z),vp[6]);
	LightTransform.Transform(CVector3D(bounds[1].X,bounds[1].Y,bounds[1].Z),vp[7]);

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
	znear=(znear<camera.GetNearPlane()+0.01f) ? camera.GetNearPlane() : znear-0.01f;
	zfar+=0.01f;

	LightProjection.SetZero();
	LightProjection._11=2/(right-left);
	LightProjection._22=2/(top-bottom);
	LightProjection._33=2/(zfar-znear);
	LightProjection._14=-(right+left)/(right-left);
	LightProjection._24=-(top+bottom)/(top-bottom);
	LightProjection._34=-(zfar+znear)/(zfar-znear);
	LightProjection._44=1;

	// calculate texture matrix
	CMatrix3D tmp2;
	CMatrix3D texturematrix;

	float dx=0.5f*float(g_Renderer.GetWidth())/float(Width);
	float dy=0.5f*float(g_Renderer.GetHeight())/float(Height);
	TextureMatrix.SetTranslation(dx,dy,0);				// transform (-0.5, 0.5) to (0,1) - texture space
	tmp2.SetScaling(dx,dy,0);					// scale (-1,1) to (-0.5,0.5)
	TextureMatrix = TextureMatrix*tmp2;
	
	TextureMatrix = TextureMatrix * LightProjection;		// transform light -> projected light space (-1 to 1)
	TextureMatrix = TextureMatrix * LightTransform;			// transform world -> light space

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
// Prepare for the next frame: Matrix calculations and texture creation if necessary
void ShadowMap::SetupFrame(const CBound& visibleBounds)
{
	m->CalcShadowMatrices(visibleBounds);
	
	if (!m->Texture)
	{
		// get shadow map size as next power of two up from view width and height
		m->Width = g_Renderer.GetWidth();
		m->Width = RoundUpToPowerOf2(m->Width);
		m->Height = g_Renderer.GetHeight();
		m->Height = RoundUpToPowerOf2(m->Height);

		// create texture object - initially filled with white, so clamp to edge clamps to correct color
		glGenTextures(1, &m->Texture);
		g_Renderer.BindTexture(0, m->Texture);

		u32 size = m->Width*m->Height;
		u32* buf=new u32[size];
		for (uint i=0;i<size;i++) buf[i]=0x00ffffff;
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,m->Width,m->Height,0,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		delete[] buf;

		// set texture parameters
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Set up to render into shadow map texture
void ShadowMap::BeginRender()
{
	// HACK HACK: this depends in non-obvious ways on the behaviour of the caller
	
	CRenderer& renderer = g_Renderer;
	int renderWidth = renderer.GetWidth();
	int renderHeight = renderer.GetHeight();
	
	// clear buffers
	glClearColor(1,1,1,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// setup viewport
	glViewport(0, 0, renderWidth, renderHeight);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(&m->LightProjection._11);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&m->LightTransform._11);

	glEnable(GL_SCISSOR_TEST);
	glScissor(1,1, renderWidth-2, renderHeight-2);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Finish rendering into shadow map texture
void ShadowMap::EndRender()
{
	glDisable(GL_SCISSOR_TEST);

	// copy result into shadow map texture
	g_Renderer.BindTexture(0, m->Texture);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, g_Renderer.GetWidth(), g_Renderer.GetHeight());

	// restore matrix stack
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Retrieve the texture handle and texture matrix for shadowing
GLuint ShadowMap::GetTexture()
{
	return m->Texture;
}

const CMatrix3D& ShadowMap::GetTextureMatrix()
{
	return m->TextureMatrix;
}
