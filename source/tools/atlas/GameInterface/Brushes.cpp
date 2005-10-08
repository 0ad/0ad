#include "precompiled.h"

#include "Brushes.h"

#include "ps/Game.h"
#include "graphics/Terrain.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"

using namespace AtlasMessage;

Brush::Brush()
: m_W(0), m_H(0), m_Data(NULL), m_Enabled(false)
{
}

Brush::~Brush()
{
	delete[] m_Data;
}

void Brush::SetData(int w, int h, const float* data)
{
	m_W = w;
	m_H = h;

	delete[] m_Data;
	m_Data = data;
}

void Brush::GetBottomRight(int& x, int& y) const
{
	CVector3D c = m_Centre;
	if (m_W % 2) c.X += CELL_SIZE/2.f;
	if (m_H % 2) c.Z += CELL_SIZE/2.f;
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	i32 cx, cy;
	terrain->CalcFromPosition(c, cx, cy);

	x = cx - (m_W-1)/2;
	y = cy - (m_H-1)/2;
}

void Brush::SetRenderEnabled(bool enabled)
{
	m_Enabled = enabled;
}

void Brush::Render()
{
	if (! m_Enabled)
		return;

	glPointSize(4.f); // TODO: does this clobber state that other people expect to stay unchanged?
	glDisable(GL_DEPTH_TEST);

	glBegin(GL_POINTS);

	int x0, y0;
	GetBottomRight(x0, y0);

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	for (int dy = 0; dy < m_H; ++dy)
	{
		for (int dx = 0; dx < m_W; ++dx)
		{
			glColor3f(0.f, clamp(m_Data[dx + dy*m_W], 0.f, 1.f), 0.f);

			CVector3D pos;
			terrain->CalcPosition(x0+dx, y0+dy, pos);
			glVertex3f(pos.X, pos.Y, pos.Z);
			//debug_printf("%f %f %f\n", pos.X, pos.Y, pos.Z);
		}
	}

	glEnd();

	glEnable(GL_DEPTH_TEST);
}

Brush AtlasMessage::g_CurrentBrush;
