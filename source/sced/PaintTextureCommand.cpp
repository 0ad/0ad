#include "PaintTextureCommand.h"
#include "UIGlobals.h"
#include "MiniMap.h"
#include "textureEntry.h"
#include "Terrain.h"

extern CTerrain g_Terrain;

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}

CPaintTextureCommand::CPaintTextureCommand(CTextureEntry* tex,int brushSize,int selectionCentre[2])
{
	m_Texture=tex;
	m_BrushSize=brushSize;
	m_SelectionCentre[0]=selectionCentre[0];
	m_SelectionCentre[1]=selectionCentre[1];
}


CPaintTextureCommand::~CPaintTextureCommand()
{
}



void CPaintTextureCommand::Execute()
{
	int r=m_BrushSize;
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	u32 mapSize=g_Terrain.GetVerticesPerSide();

	// get range of tiles affected by brush
	int x0=clamp(m_SelectionCentre[0]-r,0,mapSize-1);
	int x1=clamp(m_SelectionCentre[0]+r+1,0,mapSize-1);
	int z0=clamp(m_SelectionCentre[1]-r,0,mapSize-1);
	int z1=clamp(m_SelectionCentre[1]+r+1,0,mapSize-1);

	// iterate through tiles affected by brush
	for (int j=m_SelectionCentre[1]-r;j<=m_SelectionCentre[1]+r;j++) {
		for (int i=m_SelectionCentre[0]-r;i<=m_SelectionCentre[0]+r;i++) {

			// try and get minipatch, if there is one
			CMiniPatch* nmp=g_Terrain.GetTile(i,j);
			
			if (nmp) {
				nmp->Tex1=m_Texture ? m_Texture->m_Handle : 0;
				nmp->Tex1Priority=m_Texture ? ((int) m_Texture->m_Type) : 0;
			}
		}
	}

	// invalidate affected patches
	int px0=clamp(-1+(x0/16),0,patchesPerSide);
	int px1=clamp(1+(x1/16),0,patchesPerSide);
	int pz0=clamp(-1+(z0/16),0,patchesPerSide);
	int pz1=clamp(1+(z1/16),0,patchesPerSide);
	for (j=pz0;j<pz1;j++) {
		for (int i=px0;i<px1;i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			patch->SetDirty(RENDERDATA_UPDATE_INDICES);
		}
	}

	// rebuild this bit of the minimap
	int w=1+2*m_BrushSize;
	int x=m_SelectionCentre[1]-m_BrushSize;
	if (x<0) {
		w+=x;
		x=0;
	}
	int h=1+2*m_BrushSize;
	int y=m_SelectionCentre[0]-m_BrushSize;
	if (y<0) {
		h+=y;
		y=0;
	}
	g_MiniMap.Rebuild(x,y,w,h);
}

void CPaintTextureCommand::Undo()
{
	ApplyDataToSelection(m_DataIn);
}

void CPaintTextureCommand::Redo()
{
	ApplyDataToSelection(m_DataOut);
}

void CPaintTextureCommand::ApplyDataToSelection(const CArray2D<TextureSet>& data)
{
}
