#ifndef INCLUDED_BRUSHES
#define INCLUDED_BRUSHES

#include "maths/Vector3D.h"

class TerrainOverlay;

namespace AtlasMessage {

struct Brush
{
	Brush();
	~Brush();

	void SetData(int w, int h, const std::vector<float>& data);
	
	void SetRenderEnabled(bool enabled); // initial state is disabled

	void GetCentre(int& x, int& y) const;
	void GetBottomLeft(int& x, int& y) const;
	void GetTopRight(int& x, int& y) const;

	float Get(int x, int y) const
	{
		debug_assert(x >= 0 && x < m_W && y >= 0 && y < m_H);
		return m_Data[x + y*m_W];
	}

	int m_W, m_H;
	CVector3D m_Centre;
private:
	TerrainOverlay* m_TerrainOverlay; // NULL if rendering is not enabled
	std::vector<float> m_Data;
};

extern Brush g_CurrentBrush;

}

#endif // INCLUDED_BRUSHES
