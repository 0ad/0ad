#ifndef _TRIANGLE_H
#define _TRIANGLE_H

// necessary includes
#include "Vector3D.h"


/////////////////////////////////////////////////////////////////////////////////////////
class Triangle 
{
public:
	Triangle();
    Triangle(const CVector3D& p0,const CVector3D& p1,const CVector3D& p2);

    bool RayIntersect(const CVector3D& origin,const CVector3D& dir,float& maxdist) const;

private:
    CVector3D _vertices[3];
	CVector3D _edge[2];
};
/////////////////////////////////////////////////////////////////////////////////////////


#endif
