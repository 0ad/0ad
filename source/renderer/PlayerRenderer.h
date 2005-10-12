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
	// add object to render in deferred transparency pass
	void Add(CModel* model);
	// render all deferred objects 
	void Render();
	// render shadows from all deferred objects 
	void RenderShadows();
	// empty object list
	void Clear();

private:
	// fast render path
	void RenderFast();
	// slow render path
	void RenderSlow();
	// render given streams on all objects
	void RenderObjectsStreams(u32 streamflags, bool iscolorpass=false, u32 mflags=0);
	// list of objects to render
	std::vector<CModel*> m_Objects;
};

extern CPlayerRenderer g_PlayerRenderer;

#endif
