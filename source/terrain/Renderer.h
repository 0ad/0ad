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
#include "ogl.h"
#include "Frustum.h"

// necessary declarations
class CCamera;
class CPatch;
class CModel;
class CSprite;
class CParticleSys;
class COverlay;
class CMaterial;
class CLightEnv;
class SPatchRData;
class CTexture;

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
	// various enumerations 
	enum ETerrainMode { WIREFRAME, FILL };

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
	void Submit(CModel* model,CMatrix3D* transform);
	void Submit(CSprite* sprite,CMatrix3D* transform);
	void Submit(CParticleSys* psys,CMatrix3D* transform);
	void Submit(COverlay* overlay);

#if 0
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
#endif

	// set the current lighting environment; (note: the passed pointer is just copied to a variable within the renderer,
	// so the lightenv passed must be scoped such that it is not destructed until after the renderer is no longer rendering)
	void SetLightEnv(CLightEnv* lightenv);

	// set the mode to render subsequent terrain patches
	void SetTerrainMode(ETerrainMode mode) { m_TerrainMode=mode; }
	// get the mode to render subsequent terrain patches
	ETerrainMode GetTerrainMode() const { return m_TerrainMode; }

	// try and load the given texture
	bool LoadTexture(CTexture* texture);
	// set the given unit to reference the given texture; pass a null texture to disable texturing on any unit
	// note - active texture always set to given unit on exit
	void SetTexture(int unit,CTexture* texture);

protected:
	// patch rendering stuff
	void RenderPatchBase(CPatch* patch);
	void RenderPatchTrans(CPatch* patch);

	// model rendering stuff
	void RenderModel(SSubmission<CModel*>& modelsub);

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
	ETerrainMode m_TerrainMode;
	// submitted object lists for batching
	std::vector<SSubmission<CPatch*> > m_TerrainPatches;
	std::vector<SSubmission<CModel*> > m_Models;
	std::vector<SSubmission<CSprite*> > m_Sprites;
	std::vector<SSubmission<CParticleSys*> > m_ParticleSyses;
	std::vector<SSubmission<COverlay*> > m_Overlays;
	// current lighting setup
	CLightEnv* m_LightEnv;
};


#endif
