#include "AlterElevationCommand.h"
#include "UIGlobals.h"
#include "MiniMap.h"
#include "Terrain.h"

extern CTerrain g_Terrain;

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}


CAlterElevationCommand::CAlterElevationCommand(int brushSize,int selectionCentre[2])
{
	m_BrushSize=brushSize;
	m_SelectionCentre[0]=selectionCentre[0];
	m_SelectionCentre[1]=selectionCentre[1];
}


CAlterElevationCommand::~CAlterElevationCommand()
{
}

void CAlterElevationCommand::Execute()
{
	int r=m_BrushSize;
	u32 mapSize=g_Terrain.GetVerticesPerSide();

	// get range of vertices affected by brush
	int x0=clamp(m_SelectionCentre[0]-r,0,mapSize-1);
	int x1=clamp(m_SelectionCentre[0]+r+1,0,mapSize-1);
	int z0=clamp(m_SelectionCentre[1]-r,0,mapSize-1);
	int z1=clamp(m_SelectionCentre[1]+r+1,0,mapSize-1);
	
	// resize input/output arrays
	m_DataIn.resize(x1-x0+1,z1-z0+1);
	m_DataOut.resize(x1-x0+1,z1-z0+1);

	// fill input data
	int i,j;
	for (j=z0;j<=z1;j++) {
		for (i=x0;i<=x1;i++) {
			u16 input=g_Terrain.GetHeightMap()[j*mapSize+i];
			m_DataIn(i-x0,j-z0)=input;
		}
	}

	// call on base classes to fill the output data
	CalcDataOut(x0,x1,z0,z1);

	// now actually apply data to terrain
	ApplyDataToSelection(m_DataOut);
}


void CAlterElevationCommand::ApplyDataToSelection(const CArray2D<u16>& data)
{
	u32 mapSize=g_Terrain.GetVerticesPerSide();

	int r=m_BrushSize;
	int x0=clamp(m_SelectionCentre[0]-r,0,mapSize-1);
	int x1=clamp(m_SelectionCentre[0]+r+1,0,mapSize-1);
	int z0=clamp(m_SelectionCentre[1]-r,0,mapSize-1);
	int z1=clamp(m_SelectionCentre[1]+r+1,0,mapSize-1);


	// copy given data to heightmap
	int i,j;
	for (j=z0;j<=z1;j++) {
		for (i=x0;i<=x1;i++) {
			int idx=j*mapSize+i;
			u16 height=data(i-x0,j-z0);
			// update heightmap
			g_Terrain.GetHeightMap()[idx]=height;
		}
	}

	// flag vertex data as dirty for affected patches, and rebuild bounds of these patches
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	int px0=clamp(-1+(x0/16),0,patchesPerSide);
	int px1=clamp(1+(x1/16),0,patchesPerSide);
	int pz0=clamp(-1+(z0/16),0,patchesPerSide);
	int pz1=clamp(1+(z1/16),0,patchesPerSide);
	for (j=pz0;j<pz1;j++) {
		for (int i=px0;i<px1;i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			patch->CalcBounds();
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
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


void CAlterElevationCommand::Undo()
{
	ApplyDataToSelection(m_DataIn);
}

void CAlterElevationCommand::Redo()
{
	ApplyDataToSelection(m_DataOut);
}
