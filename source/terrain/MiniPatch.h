#ifndef MINIPATCH_H
#define MINIPATCH_H

#include "res/res.h"

class CPatch;

class CMiniPatch
{
public:
	CMiniPatch();
	~CMiniPatch();

	// get the index of this tile in the root terrain object; x,y in [0,MapSize)
	void GetTileIndex(u32& x,u32& z);

	Handle Tex1;
	int Tex1Priority;

//	Handle Tex2;
//	Handle m_AlphaMap;
//	unsigned int m_AlphaMapFlags;
		
	CPatch	*m_Parent;
//		STerrainVertex	*m_pVertices;
};


#endif
