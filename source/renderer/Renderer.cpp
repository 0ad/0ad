
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
#include "Terrain.h"
#include "Matrix3D.h"
#include "MathUtil.h"
#include "Camera.h"
#include "PatchRData.h"
#include "Texture.h"
#include "LightEnv.h"
#include "Terrain.h"
#include "CLogger.h"
#include "ps/Game.h"
#include "Profile.h"
#include "Game.h"
#include "World.h"
#include "Player.h"
#include "LOSManager.h"

#include "Model.h"
#include "ModelDef.h"

#include "ogl.h"
#include "lib/res/res.h"
#include "lib/res/file/file.h"
#include "lib/res/graphics/tex.h"
#include "lib/res/graphics/ogl_tex.h"
#include "ps/Loader.h"

#include "renderer/FixedFunctionModelRenderer.h"
#include "renderer/HWLightingModelRenderer.h"
#include "renderer/ModelRenderer.h"
#include "renderer/PlayerRenderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/RenderPathVertexShader.h"
#include "renderer/TransparencyRenderer.h"

#define LOG_CATEGORY "graphics"

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

	m_SortAllTransparent = false;
	m_FastNormals = true;
	
	m_VertexShader = 0;
	
	m_Options.m_NoVBO=false;
	m_Options.m_Shadows=true;
	m_Options.m_ShadowColor=RGBAColor(0.4f,0.4f,0.4f,1.0f);
	m_Options.m_RenderPath = RP_DEFAULT;

	for (uint i=0;i<MaxTextureUnits;i++) {
		m_ActiveTextures[i]=0;
	}

	// Must query card capabilities before creating renderers that depend
	// on card capabilities.
	EnumCaps();

	m_VertexShader = new RenderPathVertexShader;
	if (!m_VertexShader->Init())
	{
		delete m_VertexShader;
		m_VertexShader = 0;
	}

	// model rendering
	m_Models.NormalFF = new FixedFunctionModelRenderer;
	m_Models.PlayerFF = new FixedFunctionModelRenderer;
	m_Models.TransparentFF = new FixedFunctionModelRenderer;
	if (HWLightingModelRenderer::IsAvailable())
	{
		m_Models.NormalHWLit = new HWLightingModelRenderer;
		m_Models.PlayerHWLit = new HWLightingModelRenderer;
		m_Models.TransparentHWLit = new HWLightingModelRenderer;
	}
	else
	{
		m_Models.NormalHWLit = NULL;
		m_Models.PlayerHWLit = NULL;
		m_Models.TransparentHWLit = NULL;
	}
	m_Models.Transparency = new TransparencyRenderer;

	m_Models.ModWireframe = RenderModifierPtr(new WireframeRenderModifier);
	m_Models.ModPlain = RenderModifierPtr(new PlainRenderModifier);
	SetFastPlayerColor(true);
	m_Models.ModSolidColor = RenderModifierPtr(new SolidColorRenderModifier);
	m_Models.ModTransparent = RenderModifierPtr(new TransparentRenderModifier);
	m_Models.ModTransparentShadow = RenderModifierPtr(new TransparentShadowRenderModifier);

	// water
	m_RenderWater = true;
	m_WaterHeight = 5.0f;
	m_WaterColor = CColor(0.3f, 0.35f, 0.7f, 1.0f);
	m_WaterFullDepth = 4.0f;
	m_WaterMaxAlpha = 0.85f;
	m_WaterAlphaOffset = -0.05f;
	m_SWaterTrans=0;
	m_TWaterTrans=0;
	m_SWaterSpeed=0.0015f;
	m_TWaterSpeed=0.0015f;
	m_SWaterScrollCounter=0;
	m_TWaterScrollCounter=0;
	m_WaterCurrentTex=0;

	cur_loading_water_tex = 0;

	ONCE( ScriptingInit(); );
}

///////////////////////////////////////////////////////////////////////////////////
// CRenderer destructor
CRenderer::~CRenderer()
{
	// model rendering
	delete m_Models.NormalFF;
	delete m_Models.PlayerFF;
	delete m_Models.TransparentFF;
	delete m_Models.NormalHWLit;
	delete m_Models.PlayerHWLit;
	delete m_Models.TransparentHWLit;
	delete m_Models.Transparency;

	// general
	delete m_VertexShader;
	m_VertexShader = 0;

	UnloadAlphaMaps();
	UnloadWaterTextures();
}


///////////////////////////////////////////////////////////////////////////////////
// EnumCaps: build card cap bits
void CRenderer::EnumCaps()
{
	// assume support for nothing
	m_Caps.m_VBO=false;
	m_Caps.m_TextureBorderClamp=false;
	m_Caps.m_GenerateMipmaps=false;
	m_Caps.m_VertexShader=false;

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
	if (0 == oglHaveExtensions(0, "GL_ARB_shader_objects", "GL_ARB_shading_language_100", 0))
	{
		if (oglHaveExtension("GL_ARB_vertex_shader"))
			m_Caps.m_VertexShader=true;
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

	GLint bits;
	glGetIntegerv(GL_DEPTH_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: depth bits %d",bits);
	glGetIntegerv(GL_STENCIL_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: stencil bits %d",bits);
	glGetIntegerv(GL_ALPHA_BITS,&bits);
	LOG(NORMAL, LOG_CATEGORY, "CRenderer::Open: alpha bits %d",bits);

	if (m_Options.m_RenderPath == RP_DEFAULT)
		SetRenderPath(m_Options.m_RenderPath);
	
	return true;
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
		case OPT_NOPBUFFER:
			// NOT IMPLEMENTED
			break;
		default:
			debug_warn("CRenderer::SetOptionBool: unknown option");
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
		default:
			debug_warn("CRenderer::GetOptionBool: unknown option");
			break;
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
		default:
			debug_warn("CRenderer::SetOptionColor: unknown option");
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
	default:
		debug_warn("CRenderer::SetOptionFloat: unknown option");
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
		default:
			debug_warn("CRenderer::GetOptionColor: unknown option");
			break;
	}

	return defaultColor;
}


//////////////////////////////////////////////////////////////////////////////////////////
// SetRenderPath: Select the preferred render path.
// This may only be called before Open(), because the layout of vertex arrays and other
// data may depend on the chosen render path.
void CRenderer::SetRenderPath(RenderPath rp)
{
	if (rp == RP_DEFAULT)
	{
		if (m_Models.NormalHWLit && m_Models.PlayerHWLit)
			rp = RP_VERTEXSHADER;
		else
			rp = RP_FIXED;
	}
	
	if (rp == RP_VERTEXSHADER)
	{
		if (!m_Models.NormalHWLit || !m_Models.PlayerHWLit)
		{
			LOG(WARNING, LOG_CATEGORY, "Falling back to fixed function\n");
			rp = RP_FIXED;
		}
	}
	
	m_Options.m_RenderPath = rp;
}


CStr CRenderer::GetRenderPathName(RenderPath rp)
{
	switch(rp) {
	case RP_DEFAULT: return "default";
	case RP_FIXED: return "fixed";
	case RP_VERTEXSHADER: return "vertexshader";
	default: return "(invalid)";
	}
}

CRenderer::RenderPath CRenderer::GetRenderPathByName(CStr name)
{
	if (name == "fixed")
		return RP_FIXED;
	if (name == "vertexshader")
		return RP_VERTEXSHADER;
	if (name == "default")
		return RP_DEFAULT;
	
	LOG(WARNING, LOG_CATEGORY, "Unknown render path name '%hs', assuming 'default'", name.c_str());
	return RP_DEFAULT;
}


//////////////////////////////////////////////////////////////////////////////////////////
// SetFastPlayerColor
void CRenderer::SetFastPlayerColor(bool fast)
{
	m_FastPlayerColor = fast;
	
	if (m_FastPlayerColor)
	{
		if (!FastPlayerColorRender::IsAvailable())
		{
			LOG(WARNING, LOG_CATEGORY, "Falling back to slower player color rendering.");
			m_FastPlayerColor = false;
		}
	}
	
	if (m_FastPlayerColor)
		m_Models.ModPlayer = RenderModifierPtr(new FastPlayerColorRender);
	else
		m_Models.ModPlayer = RenderModifierPtr(new SlowPlayerColorRender);
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

	if (m_VertexShader)
		m_VertexShader->BeginFrame();
	
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

	glEnable(GL_SCISSOR_TEST);
	glScissor(1,1,m_Width-2,m_Height-2);

	glColor4fv(m_Options.m_ShadowColor);

	glDisable(GL_CULL_FACE);

	m_Models.NormalFF->Render(m_Models.ModSolidColor, MODELFLAG_CASTSHADOWS);
	m_Models.PlayerFF->Render(m_Models.ModSolidColor, MODELFLAG_CASTSHADOWS);
	if (m_Models.NormalHWLit)
		m_Models.NormalHWLit->Render(m_Models.ModSolidColor, MODELFLAG_CASTSHADOWS);
	if (m_Models.PlayerHWLit)
		m_Models.PlayerHWLit->Render(m_Models.ModSolidColor, MODELFLAG_CASTSHADOWS);
	m_Models.TransparentFF->Render(m_Models.ModTransparentShadow, MODELFLAG_CASTSHADOWS);
	if (m_Models.TransparentHWLit)
		m_Models.TransparentHWLit->Render(m_Models.ModTransparentShadow, MODELFLAG_CASTSHADOWS);
	m_Models.Transparency->Render(m_Models.ModTransparentShadow, MODELFLAG_CASTSHADOWS);

	glEnable(GL_CULL_FACE);

	glColor3f(1.0f,1.0f,1.0f);

	glDisable(GL_SCISSOR_TEST);

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
	int mapSize = terrain->GetVerticesPerSide();
	CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDepthMask(false);

	float time = (float) get_time();

	float period = 1.6f;
	int curTex = (int)(fmod(time, period)*(60/period));
	ogl_tex_bind(m_WaterTexture[curTex], 0);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	float tx = -fmod(time, 20.0f)/20.0f;
	float ty = fmod(time, 35.0f)/35.0f;
	glTranslatef(tx, ty, 0);

	glActiveTextureARB(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	
	// Set the proper LOD bias
	glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, m_Options.m_LodBias);

	glBegin(GL_QUADS);

	for(size_t i=0; i<m_VisiblePatches.size(); i++) 
	{
		CPatch* patch = m_VisiblePatches[i];

		for(int dx=0; dx<PATCH_SIZE; dx++) 
		{
			for(int dz=0; dz<PATCH_SIZE; dz++) 
			{
				int x = (patch->m_X*PATCH_SIZE + dx);
				int z = (patch->m_Z*PATCH_SIZE + dz);

				// is any corner of the tile below the water height? if not, no point rendering it
				bool shouldRender = false;
				for(int j=0; j<4; j++) 
				{
					float terrainHeight = terrain->getVertexGroundLevel(x + DX[j], z + DZ[j]);
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
					int ix = x + DX[j];
					int iz = z + DZ[j];

					float vertX = ix * CELL_SIZE;
					float vertZ = iz * CELL_SIZE;

					float terrainHeight = terrain->getVertexGroundLevel(ix, iz);

					float alpha = clamp((m_WaterHeight - terrainHeight) / m_WaterFullDepth + m_WaterAlphaOffset,
										-100.0f, m_WaterMaxAlpha);

					float losMod = 1.0f;
					for(int k=0; k<4; k++)
					{
						int tx = ix - DX[k];
						int tz = iz - DZ[k];

						if(tx >= 0 && tz >= 0 && tx <= mapSize-2 && tz <= mapSize-2)
						{
							ELOSStatus s = losMgr->GetStatus(tx, tz, g_Game->GetLocalPlayer());
							if(s==LOS_EXPLORED && losMod > 0.7f) 
								losMod = 0.7f;
							else if(s==LOS_UNEXPLORED && losMod > 0.0f)
								losMod = 0.0f;
						}
					}

					glColor4f(m_WaterColor.r*losMod, m_WaterColor.g*losMod, m_WaterColor.b*losMod, alpha);	
					glMultiTexCoord2fARB(GL_TEXTURE0, vertX/16.0f, vertZ/16.0f);
					glVertex3f(vertX, m_WaterHeight, vertZ);
				}
			}	//end of x loop
		}	//end of z loop
	}
	
	glEnd();

	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);

	glDepthMask(true);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void CRenderer::RenderModels()
{
	PROFILE( "render models ");

	// switch on wireframe if we need it
	if (m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}

	m_Models.NormalFF->Render(m_Models.ModPlain, 0);
	m_Models.PlayerFF->Render(m_Models.ModPlayer, 0);
	if (m_Models.NormalHWLit)
		m_Models.NormalHWLit->Render(m_Models.ModPlain, 0);
	if (m_Models.PlayerHWLit)
		m_Models.PlayerHWLit->Render(m_Models.ModPlayer, 0);

	if (m_ModelRenderMode==WIREFRAME) {
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (m_ModelRenderMode==EDGED_FACES) {
		m_Models.NormalFF->Render(m_Models.ModWireframe, 0);
		m_Models.PlayerFF->Render(m_Models.ModWireframe, 0);
		if (m_Models.NormalHWLit)
			m_Models.NormalHWLit->Render(m_Models.ModWireframe, 0);
		if (m_Models.PlayerHWLit)
			m_Models.PlayerHWLit->Render(m_Models.ModWireframe, 0);
	}
}

void CRenderer::RenderTransparentModels()
{
	PROFILE( "render transparent models ");

	// switch on wireframe if we need it
	if (m_ModelRenderMode==WIREFRAME) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}

	m_Models.TransparentFF->Render(m_Models.ModTransparent, 0);
	if (m_Models.TransparentHWLit)
		m_Models.TransparentHWLit->Render(m_Models.ModTransparent, 0);
	m_Models.Transparency->Render(m_Models.ModTransparent, 0);

	if (m_ModelRenderMode==WIREFRAME) {
		// switch wireframe off again
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else if (m_ModelRenderMode==EDGED_FACES) {
		m_Models.TransparentFF->Render(m_Models.ModWireframe, 0);
		if (m_Models.TransparentHWLit)
			m_Models.TransparentHWLit->Render(m_Models.ModWireframe, 0);
		m_Models.Transparency->Render(m_Models.ModWireframe, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FlushFrame: force rendering of any batched objects
void CRenderer::FlushFrame()
{
#ifndef SCED
	if(!g_Game || !g_Game->IsGameStarted())
		return;
#endif

	oglCheck();

	// Prepare model renderers
	PROFILE_START("prepare models");
	m_Models.NormalFF->PrepareModels();
	m_Models.PlayerFF->PrepareModels();
	m_Models.TransparentFF->PrepareModels();
	if (m_Models.NormalHWLit)
		m_Models.NormalHWLit->PrepareModels();
	if (m_Models.PlayerHWLit)
		m_Models.PlayerHWLit->PrepareModels();
	if (m_Models.TransparentHWLit)
		m_Models.TransparentHWLit->PrepareModels();
	m_Models.Transparency->PrepareModels();
	PROFILE_END("prepare models");

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

	// call on the transparency renderer to render all the transparent stuff
	MICROLOG(L"render transparent");
	RenderTransparentModels();
	oglCheck();

	// render water (note: we're assuming there's no transparent stuff over water...
	// we could also do this above render transparent if we assume there's no transparent
	// stuff underwater)
	MICROLOG(L"render water");
	RenderWater();
	oglCheck();

	// empty lists
	MICROLOG(L"empty lists");
	CPatchRData::ClearSubmissions();
	m_VisiblePatches.clear();
	
	// Finish model renderers
	m_Models.NormalFF->EndFrame();
	m_Models.PlayerFF->EndFrame();
	m_Models.TransparentFF->EndFrame();
	if (m_Models.NormalHWLit)
		m_Models.NormalHWLit->EndFrame();
	if (m_Models.PlayerHWLit)
		m_Models.PlayerHWLit->EndFrame();
	if (m_Models.TransparentHWLit)
		m_Models.TransparentHWLit->EndFrame();
	m_Models.Transparency->EndFrame();
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
	m_VisiblePatches.push_back(patch);
}

void CRenderer::Submit(CModel* model)
{
	if (model->GetFlags() & MODELFLAG_CASTSHADOWS) {
		PROFILE( "updating shadow bounds" );
		m_ShadowBound+=model->GetBounds();
	}

	if (model->GetMaterial().IsPlayer())
	{
		if (m_Options.m_RenderPath == RP_VERTEXSHADER)
			m_Models.PlayerHWLit->Submit(model);
		else
			m_Models.PlayerFF->Submit(model);
	}
	else if(model->GetMaterial().UsesAlpha())
	{
		if (m_SortAllTransparent)
			m_Models.Transparency->Submit(model);
		else if (m_Options.m_RenderPath == RP_VERTEXSHADER)
			m_Models.TransparentHWLit->Submit(model);
		else
			m_Models.TransparentFF->Submit(model);
	}
	else
	{
		if (m_Options.m_RenderPath == RP_VERTEXSHADER)
			m_Models.NormalHWLit->Submit(model);
		else
			m_Models.NormalFF->Submit(model);
	}
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
	// already tried to load this texture
	if (h)
	{
		// nothing to do here - just return success according to
		// whether this is a valid handle or not
		return h==errorhandle ? true : false;
	}

	h=ogl_tex_load(texture->GetName());
	if (h <= 0)
	{
		LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\"",(const char*) texture->GetName());
		texture->SetHandle(errorhandle);
		return false;
	}

	if(!wrapflags)
		wrapflags = GL_CLAMP_TO_EDGE;
	(void)ogl_tex_set_wrap(h, wrapflags);
	(void)ogl_tex_set_filter(h, GL_LINEAR_MIPMAP_LINEAR);

	// (this also verifies that the texture is a power-of-two)
	if(ogl_tex_upload(h) < 0)
	{
		LOG(ERROR, LOG_CATEGORY, "LoadTexture failed on \"%s\" : upload failed",(const char*) texture->GetName());
		ogl_tex_free(h);
		texture->SetHandle(errorhandle);
		return false;
	}

	texture->SetHandle(h);
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BindTexture: bind a GL texture object to current active unit
void CRenderer::BindTexture(int unit,GLuint tex)
{
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

	uint flags = 0;	// assume no alpha on failure
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
	const char* const key = "(alpha map composite)";
	Handle ht = ogl_tex_find(key);
	// alpha map texture had already been created and is still in memory:
	// reuse it, do not load again.
	if(ht > 0)
	{
		m_hCompositeAlphaMap = ht;
		return 0;
	}

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
	uint base = 0;	// texture width/height (see below)
	// for convenience, we require all alpha maps to be of the same BPP
	// (avoids another ogl_tex_get_size call, and doesn't hurt)
	uint bpp = 0;
	for(uint i=0;i<NumAlphaMaps;i++)
	{
		(void)pp_append_file(&pp, fnames[i]);
		// note: these individual textures can be discarded afterwards;
		// we cache the composite.
		textures[i] = ogl_tex_load(pp.path, RES_NO_CACHE);
		RETURN_ERR(textures[i]);

		// get its size and make sure they are all equal.
		// (the packing algo assumes this)
		uint this_width = 0, this_bpp = 0;	// fail-safe
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
	uint tile_w = 2+base+2;	// 2 pixel border (avoids bilinear filtering artifacts)
	uint total_w = RoundUpToPowerOf2(tile_w * NumAlphaMaps);
	uint total_h = base; debug_assert(is_pow2(total_h));
	u8* data=new u8[total_w*total_h*3];
	// for each tile on row
	for(uint i=0;i<NumAlphaMaps;i++)
	{
		// get src of copy
		const u8* src = 0;
		(void)ogl_tex_get_data(textures[i], (void**)&src);

		uint srcstep=bpp/8;

		// get destination of copy
		u8* dst=data+3*(i*tile_w);

		// for each row of image
		for (uint j=0;j<base;j++) {
			// duplicate first pixel
			CopyTriple(dst,src);
			dst+=3;
			CopyTriple(dst,src);
			dst+=3;

			// copy a row
			for (uint k=0;k<base;k++) {
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

	for (uint i=0;i<NumAlphaMaps;i++)
		ogl_tex_free(textures[i]);

	// upload the composite texture
	Tex t;
	(void)tex_wrap(total_w, total_h, 24, 0, data, &t);
	m_hCompositeAlphaMap = ogl_tex_wrap(&t, key);
	(void)ogl_tex_set_filter(m_hCompositeAlphaMap, GL_LINEAR);
	(void)ogl_tex_set_wrap  (m_hCompositeAlphaMap, GL_CLAMP_TO_EDGE);
	int ret = ogl_tex_upload(m_hCompositeAlphaMap, 0, 0, GL_INTENSITY);
	delete[] data;

	return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// UnloadAlphaMaps: frees the resources allocates by LoadAlphaMaps
void CRenderer::UnloadAlphaMaps()
{
	ogl_tex_free(m_hCompositeAlphaMap);
}




int CRenderer::LoadWaterTextures()
{
	const uint num_textures = ARRAY_SIZE(m_WaterTexture);

	// yield after this time is reached. balances increased progress bar
	// smoothness vs. slowing down loading.
	const double end_time = get_time() + 100e-3;

	// initialize to 0 in case something fails below
	// (we then abort the loop, but don't want undefined values in here)
	if (cur_loading_water_tex == 0)
	{
		for (uint i = 0; i < num_textures; i++)
			m_WaterTexture[i] = 0;
	}

	while (cur_loading_water_tex < num_textures)
	{
		char waterName[VFS_MAX_PATH];
		// TODO: add a member variable and setter for this. (can't make this
		// a parameter because this function is called via delay-load code)
		const char* water_type = "animation2";
		snprintf(waterName, ARRAY_SIZE(waterName), "art/textures/terrain/types/water/%s/water%02d.dds", water_type, cur_loading_water_tex+1);
		Handle ht = ogl_tex_load(waterName);
		if (ht <= 0)
		{
			LOG(ERROR, LOG_CATEGORY, "LoadWaterTextures failed on \"%s\"", waterName);
			return ht;
		}
		m_WaterTexture[cur_loading_water_tex]=ht;
		RETURN_ERR(ogl_tex_upload(ht));
		
		cur_loading_water_tex++;
		LDR_CHECK_TIMEOUT(cur_loading_water_tex, num_textures);
	}

	return 0;
}


void CRenderer::UnloadWaterTextures()
{
	for (int i = 0; i < ARRAY_SIZE(m_WaterTexture); i++)
		ogl_tex_free(m_WaterTexture[i]);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Scripting Interface

jsval CRenderer::JSI_GetFastPlayerColor(JSContext*)
{
	return ToJSVal(m_FastPlayerColor);
}

void CRenderer::JSI_SetFastPlayerColor(JSContext* ctx, jsval newval)
{
	bool fast;
	
	if (!ToPrimitive(ctx, newval, fast))
		return;
	
	SetFastPlayerColor(fast);
}

jsval CRenderer::JSI_GetRenderPath(JSContext*)
{
	return ToJSVal(GetRenderPathName(m_Options.m_RenderPath));
}

void CRenderer::JSI_SetRenderPath(JSContext* ctx, jsval newval)
{
	CStr name;
	
	if (!ToPrimitive(ctx, newval, name))
		return;
	
	SetRenderPath(GetRenderPathByName(name));
}

void CRenderer::ScriptingInit()
{
	AddProperty(L"fastPlayerColor", &CRenderer::JSI_GetFastPlayerColor, &CRenderer::JSI_SetFastPlayerColor);
	AddProperty(L"renderpath", &CRenderer::JSI_GetRenderPath, &CRenderer::JSI_SetRenderPath);
	AddProperty(L"sortAllTransparent", &CRenderer::m_SortAllTransparent);
	AddProperty(L"fastNormals", &CRenderer::m_FastNormals);

	CJSObject<CRenderer>::ScriptingInit("Renderer");
}

