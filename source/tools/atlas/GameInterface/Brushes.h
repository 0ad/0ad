#ifndef INCLUDED_BRUSHES
#define INCLUDED_BRUSHES

#include "maths/Vector3D.h"

class TerrainOverlay;

namespace AtlasMessage {

struct Brush
{
	Brush();
	~Brush();

	void SetData(ssize_t w, ssize_t h, const std::vector<float>& data);
	
	void SetRenderEnabled(bool enabled); // initial state is disabled

	void GetCentre(ssize_t& x, ssize_t& y) const;
	void GetBottomLeft(ssize_t& x, ssize_t& y) const;
	void GetTopRight(ssize_t& x, ssize_t& y) const;

	float Get(ssize_t x, ssize_t y) const
	{
		debug_assert(x >= 0 && x < m_W && y >= 0 && y < m_H);
		return m_Data[x + y*m_W];
	}

	ssize_t m_W, m_H;
	CVector3D m_Centre;
private:
	TerrainOverlay* m_TerrainOverlay; // NULL if rendering is not enabled
	std::vector<float> m_Data;
};

extern Brush g_CurrentBrush;

}

#endif // INCLUDED_BRUSHES
