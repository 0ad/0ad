/***************************************************************************************
	AUTHOR:			John M. Mena
	EMAIL:			JohnMMena@hotmail.com
	FILE:			CConsole.h
	CREATED:		1/23/05
	COMPLETED:		NULL

	DESCRIPTION:	Handles rendering all of the player objects.
					The structure was inherited from Rich Cross' Transparency Renderer.
****************************************************************************************/

#ifndef __PLAYERRENDERER_H
#define __PLAYERRENDERER_H

#include <vector>

class CModel;

class CPlayerRenderer
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
	// empty object list
	void Clear();

private:
	// render given streams on all objects
	void RenderObjectsStreams(u32 streamflags,u32 mflags=0);
	// list of transparent objects to render
	std::vector<SObject> m_Objects;
};

extern CPlayerRenderer g_PlayerRenderer;

#endif