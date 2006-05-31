#include <cxxtest/TestSuite.h>

#include "Maths/Matrix3D.h"

class TestMatrix : public CxxTest::TestSuite 
{
public:
	void test_inverse()
	{
		CMatrix3D m;
		srand(0);
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 16; ++j)
			{
				m._data[j] = -1.0f + 2.0f*(rand()/(float)RAND_MAX);
			}
			CMatrix3D n;
			m.GetInverse(n);
			m *= n;
			// verify identity has 1s on diagonal and 0 otherwise
			for (int x = 0; x < 4; ++x)
			{
				for (int y = 0; y < 4; ++y)
				{
					const float expected = (x==y)? 1.0f : 0.0f;
					TS_ASSERT_DELTA(m(x,y), expected, 0.0001f);
				}
			}
		}
	}

	void test_quats()
	{
		srand(0);
		for (int i = 0; i < 4; ++i)
		{
			CQuaternion q;
			q.FromEulerAngles(
				-6.28f + 12.56f*(rand()/(float)RAND_MAX),
				-6.28f + 12.56f*(rand()/(float)RAND_MAX),
				-6.28f + 12.56f*(rand()/(float)RAND_MAX)
				);
			CMatrix3D m;
			q.ToMatrix(m);
			CQuaternion q2 = m.GetRotation();

			// I hope there's a good reason why they're sometimes negated, and
			// it's not just a bug...
			const bool ok_oneway = 
				feq(q2.m_W, q.m_W) &&
				feq(q2.m_V.X, q.m_V.X) &&
				feq(q2.m_V.Y, q.m_V.Y) &&
				feq(q2.m_V.Z, q.m_V.Z);
			const bool ok_otherway =
				feq(q2.m_W, -q.m_W) &&
				feq(q2.m_V.X, -q.m_V.X) &&
				feq(q2.m_V.Y, -q.m_V.Y) &&
				feq(q2.m_V.Z, -q.m_V.Z);
			TS_ASSERT(ok_oneway ^ ok_otherway);
		}
	}
};

