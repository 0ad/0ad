#include "precompiled.h"

#include "SmoothElevationCommand.h"
#include "Game.h"

#include <math.h>

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}


CSmoothElevationCommand::CSmoothElevationCommand(float smoothpower,int brushSize,int selectionCentre[2])
	: CAlterElevationCommand(brushSize,selectionCentre), m_SmoothPower(smoothpower)
{
}


CSmoothElevationCommand::~CSmoothElevationCommand()
{
}

void CSmoothElevationCommand::CalcDataOut(int x0,int x1,int z0,int z1)
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	u32 mapSize=terrain->GetVerticesPerSide();

	// get valid filter vertex indices
	int fxmin=clamp(x0-2,0,mapSize-1);
	int fxmax=clamp(x1+2,0,mapSize-1);
	int fzmin=clamp(z0-2,0,mapSize-1);
	int fzmax=clamp(z1+2,0,mapSize-1);

	int i,j;
	for (j=z0;j<=z1;j++) {
		for (i=x0;i<=x1;i++) {
			// calculate output height at this pixel by filtering across neighbouring vertices
			float accum=0;
			float totalWeight=0;
			
			// iterate through each vertex in selection	
			int r=2;
			for (int k=-r;k<=r;k++) {
				for (int m=-r;m<=r;m++) {
					if (i+m>=fxmin && i+m<=fxmax && j+k>=fzmin && j+k<=fzmax) {
						float dist=sqrt(float((k*k)+(m*m)));
						float weight=1-(dist/(r+1));
						accum+=weight*terrain->GetHeightMap()[(j+k)*mapSize+(i+m)];
						totalWeight+=weight;
					}
				}
			}

			if (1 || totalWeight>0) {
				float t=0.5f;//m_SmoothPower/32.0f;
				float inputHeight=terrain->GetHeightMap()[j*mapSize+i];
				accum/=totalWeight;
				m_DataOut(i-x0,j-z0)=clamp(int((inputHeight*(1-t))+accum*t),0,65535);
			} else {
				m_DataOut(i-x0,j-z0)=terrain->GetHeightMap()[j*mapSize+i];
			}
		}
	}
}

