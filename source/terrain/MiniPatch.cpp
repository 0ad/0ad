#include "MiniPatch.h"

CMiniPatch::CMiniPatch()
{
	Tex1 = Tex2 = 0;
	m_AlphaMap = 0;
	m_pRightNeighbor = NULL;
	m_pParrent = NULL;
	m_Rotation = 0;
	m_RenderStage = 0;
	m_LastRenderedFrame = 0;
}

CMiniPatch::~CMiniPatch()
{
}

void CMiniPatch::Initialize (STerrainVertex *first_vertex)
{
	m_pVertices = first_vertex;

}