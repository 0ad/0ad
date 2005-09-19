///////////////////////////////////////////////////////////////////////////////
//
// Name:		Renderer.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
// Description: OpenGL renderer class; a higher level interface
//	on top of OpenGL to handle rendering the basic visual games 
//	types - terrain, models, sprites, particles etc
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "ogl.h"
#include "Camera.h"
#include "Frustum.h"
#include "PatchRData.h"
#include "ModelRData.h"
#include "SHCoeffs.h"
#include "Terrain.h"
#include "Singleton.h"
#include "overlay.h"

// necessary declarations
class CCamera;
class CPatch;
class CSprite;
class CParticleSys;
class COverlay;
class CMaterial;
class CLightEnv;
class CTexture;
class CTerrain;


// rendering modes
enum ERenderMode { WIREFRAME, SOLID, EDGED_FACES };

// stream flags
#define STREAM_POS		0x01
#define STREAM_NORMAL	0x02
#define STREAM_COLOR	0x04
#define STREAM_UV0		0x08
#define STREAM_UV1		0x10
#define STREAM_UV2		0x20
#define STREAM_UV3		0x40
#define STREAM_POSTOUV0 0x80

//////////////////////////////////////////////////////////////////////////////////////////
// SVertex3D: simple 3D vertex declaration
struct SVertex3D
{
	float m_Position[3];
	float m_TexCoords[2];
	unsigned int m_Color;
};

//////////////////////////////////////////////////////////////////////////////////////////
// SVertex2D: simple 2D vertex declaration
struct SVertex2D
{
	float m_Position[2];
	float m_TexCoords[2];
	unsigned int m_Color;
};

// access to sole renderer object
#define g_Renderer CRenderer::GetSingleton()

///////////////////////////////////////////////////////////////////////////////////////////
// CRenderer: base renderer class - primary interface to the rendering engine
class CRenderer : public Singleton<CRenderer>
{
private:
	std::vector<CPatch*> m_WaterPatches;

public:
	bool m_RenderWater;
	float m_WaterHeight;
	CColor m_WaterColor;
	float m_WaterMaxAlpha;
	float m_WaterFullDepth;
	float m_WaterAlphaOffset;

public:
	// various enumerations and renderer related constants
	enum { NumAlphaMaps=14 };
	enum { MaxTextureUnits=16 };
	enum Option {
		OPT_NOVBO,
		OPT_NOPBUFFER,
		OPT_SHADOWS,
		OPT_SHADOWCOLOR,
		OPT_LODBIAS
	};

	// stats class - per frame counts of number of draw calls, poly counts etc
	struct Stats {
		// set all stats to zero
		void Reset() { memset(this,0,sizeof(*this)); }
		// add given stats to this stats
		Stats& operator+=(const Stats& rhs) {
			m_Counter++;
			m_DrawCalls+=rhs.m_DrawCalls;
			m_TerrainTris+=rhs.m_TerrainTris;
			m_ModelTris+=rhs.m_ModelTris;
			m_BlendSplats+=rhs.m_BlendSplats;
			return *this;
		}
		// count of the number of stats added together 
		size_t m_Counter;
		// number of draw calls per frame - total DrawElements + Begin/End immediate mode loops
		size_t m_DrawCalls;
		// number of terrain triangles drawn
		size_t m_TerrainTris;
		// number of (non-transparent) model triangles drawn
		size_t m_ModelTris;
		// number of splat passes for alphamapping
		size_t m_BlendSplats;
	};

public:
	// constructor, destructor
	CRenderer();
	~CRenderer();

	// open up the renderer: performs any necessary initialisation
	bool Open(int width,int height,int depth);
	// shutdown the renderer: performs any necessary cleanup
	void Close();

	// resize renderer view
	void Resize(int width,int height);
	
	// set/get boolean renderer option 
	void SetOptionBool(enum Option opt,bool value);
	bool GetOptionBool(enum Option opt) const;
	// set/get RGBA color renderer option 
	void SetOptionColor(enum Option opt,const RGBAColor& value);
	void SetOptionFloat(enum Option opt, float val);
	const RGBAColor& GetOptionColor(enum Option opt) const;

	// return view width
	int GetWidth() const { return m_Width; }
	// return view height
	int GetHeight() const { return m_Height; }
	// return view aspect ratio
	float GetAspect() const { return float(m_Width)/float(m_Height); }

	// signal frame start
	void BeginFrame();
	// force rendering of any batched objects
	void FlushFrame();
	// signal frame end : implicitly flushes batched objects 
	void EndFrame();

	// set color used to clear screen in BeginFrame()
	void SetClearColor(u32 color);

	// return current frame counter
	int GetFrameCounter() const { return m_FrameCounter; }

	// set camera used for subsequent rendering operations; includes viewport, projection and modelview matrices
	void SetCamera(CCamera& camera);

	// set the viewport
	void SetViewport(const SViewPort &);

	// submission of objects for rendering; the passed matrix indicating the transform must be scoped such that it is valid beyond
	// the call to frame end, as must the object itself
	void Submit(CPatch* patch);
	void SubmitWater(CPatch* patch);
	void Submit(CModel* model);
	void Submit(CSprite* sprite);
	void Submit(CParticleSys* psys);
	void Submit(COverlay* overlay);

	// basic primitive rendering operations in 2 and 3D; handy for debugging stuff, but also useful in 
	// editor tools (eg for highlighting specific terrain patches) 
	// note: 
	//		* all 3D vertices specified in world space
	//		* primitive operations rendered immediatedly, never batched
	//		* primitives rendered in current material (set via SetMaterial)
	void RenderLine(const SVertex2D* vertices);
	void RenderLineLoop(int len,const SVertex2D* vertices);
	void RenderTri(const SVertex2D* vertices);
	void RenderQuad(const SVertex2D* vertices);
	void RenderLine(const SVertex3D* vertices);
	void RenderLineLoop(int len,const SVertex3D* vertices);
	void RenderTri(const SVertex3D* vertices);
	void RenderQuad(const SVertex3D* vertices);

	// set the current lighting environment; (note: the passed pointer is just copied to a variable within the renderer,
	// so the lightenv passed must be scoped such that it is not destructed until after the renderer is no longer rendering)
	void SetLightEnv(CLightEnv* lightenv) {
		m_LightEnv=lightenv;
	}

	// set the mode to render subsequent terrain patches
	void SetTerrainRenderMode(ERenderMode mode) { m_TerrainRenderMode=mode; }
	// get the mode to render subsequent terrain patches
	ERenderMode GetTerrainRenderMode() const { return m_TerrainRenderMode; }

	// set the mode to render subsequent models
	void SetModelRenderMode(ERenderMode mode) { m_ModelRenderMode=mode; }
	// get the mode to render subsequent models
	ERenderMode GetModelRenderMode() const { return m_ModelRenderMode; }

	// try and load the given texture
	bool LoadTexture(CTexture* texture,u32 wrapflags);
	// set the given unit to reference the given texture; pass a null texture to disable texturing on any unit;
	// active texture unit always set to given unit on exit
	void SetTexture(int unit,CTexture* texture);
	// bind a GL texture object to active unit
	void BindTexture(int unit,GLuint tex);
	// query transparency of given texture
	bool IsTextureTransparent(CTexture* texture);

	// load the default set of alphamaps; return false if any alphamap fails to load, true otherwise
	int LoadAlphaMaps();

	void UnloadAlphaMaps();

	// return stats accumulated for current frame
	const Stats& GetStats() { return m_Stats; }

    // return the current light environment
    const CLightEnv &GetLightEnv() { return *m_LightEnv; }
protected:
	friend class CVertexBuffer;
	friend class CPatchRData;
	friend class CModelRData;
	friend class CTransparencyRenderer;
	friend class CPlayerRenderer;

	// update renderdata of everything submitted
	void UpdateSubmittedObjectData();

	// patch rendering stuff
	void RenderPatchSubmissions();
	void RenderPatches();
	void RenderWater();

	// model rendering stuff
	void RenderModelSubmissions();
	void RenderModels();

	// shadow rendering stuff
	void CreateShadowMap();
	void RenderShadowMap();
	void ApplyShadowMap();
	void BuildTransformation(const CVector3D& pos,const CVector3D& right,const CVector3D& up,
						 const CVector3D& dir,CMatrix3D& result);
	void ConstructLightTransform(const CVector3D& pos,const CVector3D& lightdir,CMatrix3D& result);
	void CalcShadowMatrices();
	void CalcShadowBounds(CBound& bounds);

	// RENDERER DATA:
	// view width
	int m_Width;
	// view height
	int	m_Height;
	// view depth (bpp)
	int	m_Depth;
	// frame counter
	int m_FrameCounter;
	// current terrain rendering mode
	ERenderMode m_TerrainRenderMode;
	// current model rendering mode
	ERenderMode m_ModelRenderMode;
	// current view camera
	CCamera m_Camera;
	// color used to clear screen in BeginFrame
	float m_ClearColor[4];
	// submitted object lists for batching
	std::vector<CSprite*> m_Sprites;
	std::vector<CParticleSys*> m_ParticleSyses;
	std::vector<COverlay*> m_Overlays;
	// current lighting setup
	CLightEnv* m_LightEnv;
	// current spherical harmonic coefficients (for unit lighting), derived from lightenv
	CSHCoeffs m_SHCoeffsUnits;
	// current spherical harmonic coefficients (for terrain lighting), derived from lightenv
	CSHCoeffs m_SHCoeffsTerrain;
	// ogl_tex handle of composite alpha map (all the alpha maps packed into one texture)
	Handle m_hCompositeAlphaMap;
	// handle of shadow map
	u32 m_ShadowMap;
	// width, height of shadow map
	u32 m_ShadowMapWidth,m_ShadowMapHeight;
	// object space bound of shadow casting objects
	CBound m_ShadowBound;
	// per-frame flag: has the shadow map been rendered this frame?
	bool m_ShadowRendered;
	// projection matrix of shadow casting light
	CMatrix3D m_LightProjection;
	// transformation matrix of shadow casting light
	CMatrix3D m_LightTransform;
	// coordinates of each (untransformed) alpha map within the packed texture
	struct {
		float u0,u1,v0,v1;
	} m_AlphaMapCoords[NumAlphaMaps];
	// card capabilities
	struct Caps {
		bool m_VBO;
		bool m_TextureBorderClamp;
		bool m_GenerateMipmaps;
	} m_Caps;
	// renderer options 
	struct Options {
		bool m_NoVBO;
		bool m_Shadows;
		RGBAColor m_ShadowColor;
		float m_LodBias;
	} m_Options;
	// build card cap bits
	void EnumCaps();
	// per-frame renderer stats
	Stats m_Stats;
	// active textures on each unit
	GLuint m_ActiveTextures[MaxTextureUnits];
};


#endif
