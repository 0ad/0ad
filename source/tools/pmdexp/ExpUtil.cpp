#include "MaxInc.h"
#include "ExpUtil.h"

//////////////////////////////////////////////////////////////////////
// MAXtoGL: convert point from MAX's coordinate system to that used
// within the engine
void MAXtoGL(Point3 &pnt)
{
	float tmp = pnt.y;
	pnt.y = pnt.z;
	pnt.z = tmp;
}
