#include "MiniPatch.h"
#include "Patch.h"

CMiniPatch::CMiniPatch()
{
	Tex1 = 0;
	Tex1Priority = 0;
	m_Parent = NULL;
}

CMiniPatch::~CMiniPatch()
{
}

void CMiniPatch::GetTileIndex(u32& x,u32& z)
{
	u32 tindex=this-&m_Parent->m_MiniPatches[0][0];
	x=(m_Parent->m_X*16)+tindex%16;
	z=(m_Parent->m_Z*16)+tindex/16;
}

