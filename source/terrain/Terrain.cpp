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

#include "Terrain.H"
#include "tex.h"
#include "mem.h"

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

bool CTerrain::Initalize (char *filename)
{
	SeasonLight[0].Set (3, -1, 3);
	SeasonLight[0].Normalize();
	SeasonColor[0][0] = 0.8f; SeasonColor[0][1] = 1.0f; SeasonColor[0][2] = 0.8f;

	SeasonLight[1].Set (2, -1, -3);
	SeasonLight[1].Normalize();
	SeasonColor[1][0] = 1.0f; SeasonColor[1][1] = 0.9f; SeasonColor[1][2] = 0.9f;

TEX tex;
Handle h = tex_load(filename, &tex);
if(!h)
return false;
Handle hm = tex.hm;
MEM* mem = (MEM*)h_user_data(hm, H_MEM);
const u8* data = (const u8*)mem->p;

	m_pVertices = new STerrainVertex[MAP_SIZE*MAP_SIZE];
	if (m_pVertices == NULL)
		return false;

	for (int j=0; j<MAP_SIZE; j++)
	{
		for (int i=0; i<MAP_SIZE; i++)
		{
			int pos = j*MAP_SIZE + i;

			m_pVertices[pos].m_Position.X = ((float)i)*CELL_SIZE;
			m_pVertices[pos].m_Position.Y = (*data++)*0.35f;
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

	CalcLighting();
	SetNeighbors();

	return true;
}

void CTerrain::CalcLighting ()
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

			n[0].Normalize();
			n[1].Normalize();
			n[2].Normalize();
			n[3].Normalize();

			CVector3D Normal = n[0] + n[1] + n[2] + n[3];
			Normal.Normalize();

			float Color1 = Normal.Dot(SeasonLight[0]*-1)/(Normal.GetLength() * SeasonLight[0].GetLength());
			Color1 = (Color1+1.0f)/1.4f;

			if (Color1>1.0f)
				Color1=1.0f;
			if (Color1<0.0f)
				Color1=0.0f;

			float Color2 = Normal.Dot(SeasonLight[1]*-1)/(Normal.GetLength() * SeasonLight[1].GetLength());
			Color2 = (Color2+1.0f)/1.4f;

			if (Color2>1.0f)
				Color2=1.0f;
			if (Color2<0.0f)
				Color2=0.0f;

			m_pVertices[j*MAP_SIZE + i].m_Color[0][0] = Color1*SeasonColor[0][0];
			m_pVertices[j*MAP_SIZE + i].m_Color[0][1] = Color1*SeasonColor[0][1];
			m_pVertices[j*MAP_SIZE + i].m_Color[0][2] = Color1*SeasonColor[0][2];

			m_pVertices[j*MAP_SIZE + i].m_Color[1][0] = Color2*SeasonColor[1][0];
			m_pVertices[j*MAP_SIZE + i].m_Color[1][1] = Color2*SeasonColor[1][1];
			m_pVertices[j*MAP_SIZE + i].m_Color[1][2] = Color2*SeasonColor[1][2];

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