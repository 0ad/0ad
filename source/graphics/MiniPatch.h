///////////////////////////////////////////////////////////////////////////////
//
// Name:		MiniPatch.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MINIPATCH_H
#define _MINIPATCH_H

#include "res/handle.h"

class CPatch;

///////////////////////////////////////////////////////////////////////////////
// CMiniPatch: definition of a single terrain tile
class CMiniPatch
{
public:
	// constructor
	CMiniPatch();
	// destructor
	~CMiniPatch();

	// get the index of this tile in the root terrain object; x,y in [0,MapSize)
	void GetTileIndex(u32& x,u32& z);

public:
	// texture applied to tile
	Handle Tex1;
	// 'priority' of the texture - determines drawing order of terrain textures
	int Tex1Priority;
	// parent patch
	CPatch* m_Parent;
};


#endif
