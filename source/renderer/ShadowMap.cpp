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
#include "CLogger.h"

#include "graphics/LightEnv.h"

#include "maths/Bound.h"
#include "maths/MathUtil.h"
#include "maths/Matrix3D.h"

#include "renderer/Renderer.h"
#include "renderer/ShadowMap.h"

#define LOG_CATEGORY "graphics"


///////////////////////////////////////////////////////////////////////////////////////////////////
// ShadowMap implementation

/**
 * Struct ShadowMapInternals: Internal data for the ShadowMap implementation
 */
struct ShadowMapInternals
{
	// whether we're using depth texture or luminance map
	bool UseDepthTexture;
	// handle of shadow map
	GLuint Texture;
	// width, height of shadow map
	u32 Width, Height;
	// transform light space into projected light space
	// in projected light space, the shadowbound box occupies the [-1..1] cube
	// calculated on BeginRender, after the final shadow bounds are known
	CMatrix3D LightProjection;
	// Transform world space into light space; calculated on SetupFrame
	CMatrix3D LightTransform;
	// Transform world space into texture space of the shadow map;
	// calculated on BeginRender, after the final shadow bounds are known
	CMatrix3D TextureMatrix;

	// transform light space into world space
	CMatrix3D InvLightTransform;
	// bounding box of shadowed objects in light space
	CBound ShadowBound;

	// Helper functions
	void CalcShadowMatrices();
	void CreateTexture();
};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
ShadowMap::ShadowMap()
{
	m = new ShadowMapInternals;
	m->Texture = 0;
	m->Width = 0;
	m->Height = 0;
	m->UseDepthTexture = false;
}


ShadowMap::~ShadowMap()
{
	if (m->Texture)
		glDeleteTextures(1, &m->Texture);

	delete m;
}

//////////////////////////////////////////////////////////////////////////////
// SetCameraAndLight: camera and light direction for this frame
void ShadowMap::SetupFrame(const CCamera& camera, const CVector3D& lightdir)
{
	if (!m->Texture)
		m->CreateTexture();

	CVector3D z = lightdir;
	CVector3D y;
	CVector3D x = camera.m_Orientation.GetIn();
	CVector3D eyepos = camera.m_Orientation.GetTranslation();

	z.Normalize();
	x -= z * z.Dot(x);
	if (x.GetLength() < 0.001)
	{
		// this is invoked if the camera and light directions almost coincide
		// assumption: light direction has a significant Z component
		x = CVector3D(1.0, 0.0, 0.0);
		x -= z * z.Dot(x);
	}
	x.Normalize();
	y = z.Cross(x);

	// X axis perpendicular to light direction, flowing along with view direction
	m->LightTransform._11 = x.X;
	m->LightTransform._12 = x.Y;
	m->LightTransform._13 = x.Z;

	// Y axis perpendicular to light and view direction
	m->LightTransform._21 = y.X;
	m->LightTransform._22 = y.Y;
	m->LightTransform._23 = y.Z;

	// Z axis is in direction of light
	m->LightTransform._31 = z.X;
	m->LightTransform._32 = z.Y;
	m->LightTransform._33 = z.Z;

	// eye is at the origin of the coordinate system
	m->LightTransform._14 = -x.Dot(eyepos);
	m->LightTransform._24 = -y.Dot(eyepos);
	m->LightTransform._34 = -z.Dot(eyepos);

	m->LightTransform._41 = 0.0;
	m->LightTransform._42 = 0.0;
	m->LightTransform._43 = 0.0;
	m->LightTransform._44 = 1.0;

	m->LightTransform.GetInverse(m->InvLightTransform);
	m->ShadowBound.SetEmpty();
}


//////////////////////////////////////////////////////////////////////////////
// AddShadowedBound: add a world-space bounding box to the bounds of shadowed
// objects
void ShadowMap::AddShadowedBound(const CBound& bounds)
{
	CBound lightspacebounds;

	bounds.Transform(m->LightTransform, lightspacebounds);
	m->ShadowBound += lightspacebounds;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// CalcShadowMatrices: calculate required matrices for shadow map generation - the light's
// projection and transformation matrices
void ShadowMapInternals::CalcShadowMatrices()
{
	CRenderer& renderer = g_Renderer;

	// Setup orthogonal projection (lightspace -> clip space) for shadowmap rendering
	CVector3D scale = ShadowBound[1] - ShadowBound[0];
	CVector3D shift = (ShadowBound[1] + ShadowBound[0]) * -0.5;

	if (scale.X < 1.0)
		scale.X = 1.0;
	if (scale.Y < 1.0)
		scale.Y = 1.0;
	if (scale.Z < 1.0)
		scale.Z = 1.0;

	scale.X = 2.0 / scale.X;
	scale.Y = 2.0 / scale.Y;
	scale.Z = 2.0 / scale.Z;

	LightProjection.SetZero();
	LightProjection._11 = scale.X;
	LightProjection._14 = shift.X * scale.X;
	LightProjection._22 = scale.Y;
	LightProjection._24 = shift.Y * scale.Y;
	LightProjection._33 = scale.Z;
	LightProjection._34 = shift.Z * scale.Z;
	LightProjection._44 = 1.0;


	// Calculate texture matrix by creating the clip space to texture coordinate matrix
	// and then concatenating all matrices that have been calculated so far
	CMatrix3D clipToTex;
	float texscalex = 0.5 * (float)renderer.GetWidth() / (float)Width;
	float texscaley = 0.5 * (float)renderer.GetHeight() / (float)Height;

	clipToTex.SetZero();
	clipToTex._11 = texscalex;
	clipToTex._14 = texscalex;
	clipToTex._22 = texscaley;
	clipToTex._24 = texscaley;
	clipToTex._33 = 0.5; // translate -1..1 clip space Z values to tex Z values
	clipToTex._34 = 0.5;
	clipToTex._44 = 1.0;

	TextureMatrix = clipToTex * LightProjection * LightTransform;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Create the shadow map
void ShadowMapInternals::CreateTexture()
{
	if (Texture)
		glDeleteTextures(1, &Texture);

	// get shadow map size as next power of two up from view width and height
	Width = g_Renderer.GetWidth();
	Width = RoundUpToPowerOf2(Width);
	Height = g_Renderer.GetHeight();
	Height = RoundUpToPowerOf2(Height);

	LOG(NORMAL, LOG_CATEGORY, "Creating shadow texture (size %ix%i) (format = %s)",
		Width, Height, UseDepthTexture ? "DEPTH_COMPONENT" : "LUMINANCE");

	// create texture object
	glGenTextures(1, &Texture);
	g_Renderer.BindTexture(0, Texture);

	u32 size = Width*Height;

	if (UseDepthTexture)
	{
		float* buf = new float[size];
		for(uint i = 0; i < size; i++) buf[i] = 1.0;
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Width, Height, 0,
			     GL_DEPTH_COMPONENT, GL_FLOAT, buf);
		delete[] buf;
	}
	else
	{
		u32* buf=new u32[size];
		for (uint i=0;i<size;i++) buf[i]=0x00ffffff;
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE8,Width,Height,0,GL_RGBA,GL_UNSIGNED_BYTE,buf);
		delete[] buf;
	}

	// set texture parameters
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Set up to render into shadow map texture
void ShadowMap::BeginRender()
{
	// HACK HACK: this depends in non-obvious ways on the behaviour of the caller

	CRenderer& renderer = g_Renderer;
	int renderWidth = renderer.GetWidth();
	int renderHeight = renderer.GetHeight();

	// Calc remaining shadow matrices
	m->CalcShadowMatrices();

	// clear buffers
	if (m->UseDepthTexture)
	{
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glColorMask(0,0,0,0);
	}
	else
	{
		glClearColor(1,1,1,0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	}

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

	if (m->UseDepthTexture)
	{
		glColorMask(1,1,1,1);
	}

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


///////////////////////////////////////////////////////////////////////////////////////////////////
// Using depth textures vs. a simple luminance map
bool ShadowMap::GetUseDepthTexture()
{
	return m->UseDepthTexture;
}

void ShadowMap::SetUseDepthTexture(bool depthTexture)
{
	if (depthTexture)
	{
		if (!g_Renderer.GetCapabilities().m_DepthTextureShadows)
		{
			LOG(WARNING, LOG_CATEGORY, "Depth textures are not supported by your graphics card/driver. Fallback to luminance map (no self-shadowing)!");
			depthTexture = false;
		}
	}

	if (depthTexture != m->UseDepthTexture)
	{
		if (m->Texture)
		{
			glDeleteTextures(1, &m->Texture);
			m->Texture = 0;
		}
		m->Width = m->Height = 0;

		m->UseDepthTexture = depthTexture;
	}
}


//////////////////////////////////////////////////////////////////////////////
// RenderDebugDisplay: debug visualizations
//  - blue: objects in shadow
void ShadowMap::RenderDebugDisplay()
{
	glDepthMask(0);
	glDisable(GL_CULL_FACE);

	// Render shadow bound
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixf(&m->InvLightTransform._11);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(0,0,255,64);
	m->ShadowBound.Render();
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub(0,0,255);
	m->ShadowBound.Render();

	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(0.0, 0.0, 50.0);
	glEnd();
	glBegin(GL_POLYGON);
		glVertex3f(0.0, 0.0, 50.0);
		glVertex3f(50.0, 0.0, 50.0);
		glVertex3f(0.0, 50.0, 50.0);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();

#if 0
	CMatrix3D InvTexTransform;

	m->TextureMatrix.GetInverse(InvTexTransform);

	// Render representative texture rectangle
	glPushMatrix();
	glMultMatrixf(&InvTexTransform._11);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4ub(255,0,0,64);
	glBegin(GL_QUADS);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(1.0, 1.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
	glEnd();
	glDisable(GL_BLEND);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub(255,0,0);
	glBegin(GL_QUADS);
		glVertex3f(0.0, 0.0, 0.0);
		glVertex3f(1.0, 0.0, 0.0);
		glVertex3f(1.0, 1.0, 0.0);
		glVertex3f(0.0, 1.0, 0.0);
	glEnd();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPopMatrix();
#endif

	// Render the shadow map
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, 1.0, 1.0, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	g_Renderer.BindTexture(0, m->Texture);
	if (m->UseDepthTexture)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	}
	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
		glTexCoord2f(1.0, 0.0); glVertex2f(0.2, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex2f(0.2, 0.2);
		glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 0.2);
	glEnd();

	glEnable(GL_CULL_FACE);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
}
