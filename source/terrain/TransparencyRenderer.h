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

private:
	// list of transparent objects to render
	std::vector<SObject> m_Objects;
};

extern CTransparencyRenderer g_TransparencyRenderer;

#endif
