///////////////////////////////////////////////////////////////////////////////
//
// Name:		TransparencyRenderer.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __TRANSPARENCYRENDERER_H
#define __TRANSPARENCYRENDERER_H

#include <vector>

class CModel;

class CTransparencyRenderer
{
public:
	struct SObject {
		// the transparent model
		CModel* m_Model;
		// sqrd distance from camera to centre of nearest triangle
		float m_Dist;
	};

public:
	// add object to render in deferred transparency pass
	void Add(CModel* model);
	// render all deferred objects 
	void Render();
	// render shadows from all deferred objects 
	void RenderShadows();
	// coarsely sort submitted objects in back to front manner
	void Sort();
	// empty object list
	void Clear();

private:
	// render given streams on all objects
	void RenderObjectsStreams(u32 streamflags);
	// list of transparent objects to render
	std::vector<SObject> m_Objects;
};

extern CTransparencyRenderer g_TransparencyRenderer;

#endif
