#include "lib/self_test.h"

#include "maths/Bound.h"

class TestBound : public CxxTest::TestSuite 
{
public:
	void test_empty()
	{
		CBound bound;
		TS_ASSERT(bound.IsEmpty());
		bound += CVector3D(1, 2, 3);
		TS_ASSERT(! bound.IsEmpty());
		bound.SetEmpty();
		TS_ASSERT(bound.IsEmpty());
	}

	void test_extend_vector()
	{
		CBound bound;
		CVector3D v (1, 2, 3);
		bound += v;
		
		CVector3D centre;
		bound.GetCentre(centre);
		TS_ASSERT_EQUALS(centre, v);
	}

	void test_extend_bound()
	{
		CBound bound;
		CVector3D v (1, 2, 3);
		CBound b (v, v);
		bound += b;

		CVector3D centre;
		bound.GetCentre(centre);
		TS_ASSERT_EQUALS(centre, v);
	}
};
