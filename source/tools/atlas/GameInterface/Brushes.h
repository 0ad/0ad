#include "maths/Vector3D.h"

namespace AtlasMessage {

struct Brush
{
	Brush();
	~Brush();

	void SetData(int w, int h, const float* data);
	
	void SetRenderEnabled(bool enabled); // initial state is disabled
	void Render(); // only does anything if enabled

	void GetBottomRight(int& x, int& y) const;

	float Get(int x, int y) const
	{
		debug_assert(x >= 0 && x < m_W && y >= 0 && y < m_H);
		return m_Data[x + y*m_W];
	}

	int m_W, m_H;
	CVector3D m_Centre;
private:
	const float* m_Data;
	bool m_Enabled;
};

extern Brush g_CurrentBrush;

}
