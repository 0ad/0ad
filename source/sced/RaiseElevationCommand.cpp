#include "RaiseElevationCommand.h"
#include "Terrain.h"

extern CTerrain g_Terrain;

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}


CRaiseElevationCommand::CRaiseElevationCommand(int deltaheight,int brushSize,int selectionCentre[2])
	: CAlterElevationCommand(brushSize,selectionCentre), m_DeltaHeight(deltaheight)
{
}


CRaiseElevationCommand::~CRaiseElevationCommand()
{
}

void CRaiseElevationCommand::CalcDataOut(int x0,int x1,int z0,int z1)
{
	// fill output data
	u32 mapSize=g_Terrain.GetVerticesPerSide();
	int i,j;
	for (j=z0;j<=z1;j++) {
		for (i=x0;i<=x1;i++) {
			u32 input=g_Terrain.GetHeightMap()[j*mapSize+i];
			u16 output=clamp(input+m_DeltaHeight,0,65535);
			m_DataOut(i-x0,j-z0)=output;
		}
	}
}
