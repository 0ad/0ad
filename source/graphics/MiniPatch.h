/**
 * =========================================================================
 * File        : MiniPatch.h
 * Project     : 0 A.D.
 * Description : Definition of a single terrain tile
 * =========================================================================
 */

#ifndef INCLUDED_MINIPATCH
#define INCLUDED_MINIPATCH

#include "lib/res/handle.h"

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
	void GetTileIndex(ssize_t& x,ssize_t& z);

public:
	// texture applied to tile
	Handle Tex1;
	// 'priority' of the texture - determines drawing order of terrain textures
	int Tex1Priority;
	// parent patch
	CPatch* m_Parent;
};


#endif
