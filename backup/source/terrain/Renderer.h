//----------------------------------------------------------------
//
// Name:		Renderer.h
// Last Update: 25/11/03
// Author:		Rich Cross
// Contact:		rich@0ad.wildfiregames.com
//
// Description: OpenGL renderer class; a higher level interface
//	on top of OpenGL to handle rendering the basic visual games 
//	types - terrain, models, sprites, particles etc
//----------------------------------------------------------------


#ifndef RENDERER_H
#define RENDERER_H

#include <vector>
#include "res/res.h"
#include "ogl.h"
#include "Camera.h"
#include "Frustum.h"
#include "PatchRData.h"
#include "ModelRData.h"
#include "SHCoeffs.h"
#include "Terrain.h"

// necessary declarations
class CCamera;
class CPatch;
class CVisual;
class CSprite;
class CParticleSys;
class COverlay;
class CMaterial;
class CLightEnv;
class CTexture;
class CTerrain;


// rendering modes
enum ERenderMode { WIREFRAME, SOLID, EDGED_FACES };


//////////////////////////////////////////////////////////////////////////////////////////
// SSubmission: generalised class representating a submission of objects to renderer
template <class T>
struct SSubmission
{
	T m_Object;
	CMatrix3D* m_Transform;
};

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


///////////////////////////////////////////////////////////////////////////////////////////
// CRenderer: base renderer class - primary interface to the rendering engine
class CRenderer
{
public:
	// various enumerations and renderer related constants
	enum { NumAlphaMaps=14 };

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
			m_TransparentTris+=rhs.m_TransparentTris;
			m_BlendSplats+=rhs.m_BlendSplats;
			return *this;
		}
		// count of the number of stats added together 
		u32 m_Counter;
		// number of draw calls per frame - total DrawElements + Begin/End immediate mode loops
		u32 m_DrawCalls;
		// number of terrain triangles drawn
		u32 m_TerrainTris;
		// number of (non-transparent) model triangles drawn
		u32 m_ModelTris;
		// number of transparent model triangles drawn
		u32 m_TransparentTris;
		// number of splat passes for alphamapping
		u32 m_BlendSplats;
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

	// signal frame start
	void BeginFrame();
	// force rendering of any batched objects
	void FlushFrame();
	// signal frame end : implicitly flushes batched objects 
	void EndFrame();

	// return current frame counter
	int GetFrameCounter() const { return m_FrameCounter; }

	// set camera used for subsequent rendering operations; includes viewport, projection and modelview matrices
	void SetCamera(CCamera& camera);

	// submission of objects for rendering; the passed matrix indicating the transform must be scoped such that it is valid beyond
	// the call to frame end, as must the object itself
	void Submit(CPatch* patch);
	void Submit(CVisual* visual);
	void Submit(CSprite* sprite,CMatrix3D* transform);
	void Submit(CParticleSys* psys,CMatrix3D* transform);
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
	bool LoadTexture(CTexture* texture);
	// set the given unit to reference the given texture; pass a null texture to disable texturing on any unit
	// note - active texture always set to given unit on exit
	void SetTexture(int unit,CTexture* texture,u32 wrapflags=0);
	// query transparency of given texture
	bool IsTextureTransparent(CTexture* texture);

	// load the default set of alphamaps; return false if any alphamap fails to load, true otherwise
	bool LoadAlphaMaps(const char* fnames[]);

	// return stats accumulated for current frame
	const Stats& GetStats() { return m_Stats; }
	
	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }

protected:
	friend class CPatchRData;
	friend class CModelRData;
	friend class CTransparencyRenderer;

	// patch rendering stuff
	void RenderPatchSubmissions();
	void RenderPatches();

	// model rendering stuff
	void BuildTransparentPasses(CVisual* visual);
	void RenderModelSubmissions();
	void RenderModels();

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
	// submitted object lists for batching
	std::vector<SSubmission<CPatch*> > m_TerrainPatches;
	std::vector<SSubmission<CVisual*> > m_Models;
	std::vector<SSubmission<CSprite*> > m_Sprites;
	std::vector<SSubmission<CParticleSys*> > m_ParticleSyses;
	std::vector<SSubmission<COverlay*> > m_Overlays;
	// current lighting setup
	CLightEnv* m_LightEnv;
	// current spherical harmonic coefficients (for unit lighting), derived from lightenv
	CSHCoeffs m_SHCoeffsUnits;
	// current spherical harmonic coefficients (for terrain lighting), derived from lightenv
	CSHCoeffs m_SHCoeffsTerrain;
	// default alpha maps
	//Handle m_AlphaMaps[NumAlphaMaps];
	// all the alpha maps packed into one texture
	unsigned int m_CompositeAlphaMap;
	// coordinates of each (untransformed) alpha map within the packed texture
	struct {
		float u0,u1,v0,v1;
	} m_AlphaMapCoords[NumAlphaMaps];

	// card capabilities
	struct Caps {
		bool m_VBO;
	} m_Caps;
	// build card cap bits
	void EnumCaps();
	// per-frame renderer stats
	Stats m_Stats;
};


#endif
