#ifndef MINIPATCH_H
#define MINIPATCH_H

#include "Vector3D.h"
#include "res.h"

struct STerrainVertex
{
	CVector3D m_Position;
	float	  m_Color[2][3];
};


class CMiniPatch
{
	public:
		CMiniPatch();
		~CMiniPatch();

		void Initialize (STerrainVertex *first_vertex);
		
Handle Tex1, Tex2;
Handle m_AlphaMap;
		CMiniPatch		*m_pRightNeighbor;
		CPatch			*m_pParrent;
		unsigned char	m_RenderStage;
		unsigned int	m_LastRenderedFrame;

		unsigned char	m_Rotation;

		STerrainVertex	*m_pVertices;
};


#endif