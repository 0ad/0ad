//***********************************************************
//
// Name:		Terrain.Cpp
// Last Update: 23/2/02
// Author:		Poya Manouchehri
//
// Description: CTerrain handles the terrain portion of the
//				engine. It holds open the file to the terrain
//				information, so terrain data can be loaded
//				dynamically. We use a ROAM method to render 
//				the terrain, ie using binary triangle trees.
//				The terrain consists of smaller PATCHS, which
//				do most of the work.
//
//***********************************************************

#include "res/res.h"

#include "Terrain.h"
#include "LightEnv.h"
#include "SHCoeffs.h"

bool g_HillShading = true;

CVector3D			SeasonLight[2];
float				SeasonColor[2][3];

CTerrain::CTerrain ()
{
	m_pVertices = NULL;
}

CTerrain::~CTerrain ()
{
	delete [] m_pVertices;
}

bool CTerrain::Load(char *filename)
{
	Handle ht = tex_load(filename);
	if(!ht)
		return false;
    void* p;
	tex_info(ht, 0, 0, &p);

	return InitFromHeightmap((const u8*)p);
}

bool CTerrain::InitFromHeightmap(const u8* data)
{
	int j;

	delete[] m_pVertices;

	m_pVertices = new STerrainVertex[MAP_SIZE*MAP_SIZE];
	if (m_pVertices == NULL)
		return false;

	for (j=0; j<MAP_SIZE; j++)
	{
		for (int i=0; i<MAP_SIZE; i++)
		{
			int pos = j*MAP_SIZE + i;

			m_pVertices[pos].m_Position.X = ((float)i)*CELL_SIZE;
			m_pVertices[pos].m_Position.Y = (*data++)*HEIGHT_SCALE;
			m_pVertices[pos].m_Position.Z = ((float)j)*CELL_SIZE;
		}
	}

	for (j=0; j<NUM_PATCHES_PER_SIDE; j++)
	{
		for (int i=0; i<NUM_PATCHES_PER_SIDE; i++)
		{
			int pos = j*MAP_SIZE*PATCH_SIZE;
			pos += i*PATCH_SIZE;

			m_Patches[j][i].Initialize ( &(m_pVertices[pos]) );
		}
	}

	CalcNormals();
	SetNeighbors();

	return true;
}

void CTerrain::CalcLighting(const CLightEnv& lightEnv)
{
	CSHCoeffs coeffs;
	coeffs.AddAmbientLight(lightEnv.m_TerrainAmbientColor);
	
	CVector3D dirlight;
	lightEnv.GetSunDirection(dirlight);
	coeffs.AddDirectionalLight(dirlight,lightEnv.m_SunColor);

	for (int k=0;k<MAP_SIZE*MAP_SIZE;++k) {
		coeffs.Evaluate(m_pVertices[k].m_Normal,m_pVertices[k].m_Color);
	}
}

void CTerrain::CalcNormals()
{		
	CVector3D left, right, up, down, n[4];
	
	for (int j=0; j<MAP_SIZE; j++)
	{
		for (int i=0; i<MAP_SIZE; i++)
		{
			left.Clear();
			right.Clear();
			up.Clear();
			down.Clear();
			
			if (i>0)
				left = m_pVertices[j*MAP_SIZE + i - 1].m_Position - 
					   m_pVertices[j*MAP_SIZE + i].m_Position;

			if (i<MAP_SIZE-1)
				right = m_pVertices[j*MAP_SIZE + i + 1].m_Position - 
					    m_pVertices[j*MAP_SIZE + i].m_Position;

			if (j>0)
				up = m_pVertices[(j-1)*MAP_SIZE + i].m_Position - 
				     m_pVertices[j*MAP_SIZE + i].m_Position;

			if (j<MAP_SIZE-1)
				down = m_pVertices[(j+1)*MAP_SIZE + i].m_Position - 
					   m_pVertices[j*MAP_SIZE + i].m_Position;

			n[0] = up.Cross(left);
			n[1] = left.Cross(down);
			n[2] = down.Cross(right);
			n[3] = right.Cross(up);

			float n0len=n[0].GetLength();
			if (n0len>0.0001f) n[0]*=1.0f/n0len;

			float n1len=n[1].GetLength();
			if (n1len>0.0001f) n[1]*=1.0f/n1len;

			float n2len=n[2].GetLength();
			if (n2len>0.0001f) n[2]*=1.0f/n2len;

			float n3len=n[3].GetLength();
			if (n3len>0.0001f) n[3]*=1.0f/n3len;

			CVector3D Normal = n[0] + n[1] + n[2] + n[3];
			float nlen=Normal.GetLength();
			if (nlen>0.00001f) Normal*=1.0f/nlen;
			
			m_pVertices[j*MAP_SIZE + i].m_Normal=Normal;
		}
	}
}

void CTerrain::SetNeighbors ()
{
	CPatch *ThisPatch, *RightPatch;

	for (int pj=0; pj<NUM_PATCHES_PER_SIDE; pj++)
	{
		for (int pi=0; pi<NUM_PATCHES_PER_SIDE; pi++)
		{
			ThisPatch = &m_Patches[pj][pi];
			
			if (pi < NUM_PATCHES_PER_SIDE-1)
				RightPatch = &m_Patches[pj][pi+1]; 
			else
				RightPatch = NULL;


			for (int tj=0; tj<16; tj++)
			{
				for (int ti=0; ti<16; ti++)
				{
					CMiniPatch *MPatch = &ThisPatch->m_MiniPatches[tj][ti];

					MPatch->m_pParrent = ThisPatch;

					if (ti < 15)
						MPatch->m_pRightNeighbor = &ThisPatch->m_MiniPatches[tj][ti+1];
					else
					{
						if (RightPatch)
							MPatch->m_pRightNeighbor = &RightPatch->m_MiniPatches[tj][0];
						else
							MPatch->m_pRightNeighbor = NULL;
					}
				}
			}
		}
	}

}
