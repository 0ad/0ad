#include "Triangle.h"
#include "Plane.h"

#define EPSILON 0.00001f

Triangle::Triangle()
{
}

Triangle::Triangle(const CVector3D& p0,const CVector3D& p1,const CVector3D& p2)
{
	_vertices[0]=p0;
	_vertices[1]=p1;
	_vertices[2]=p2;

    // calculate edge vectors
	_edge[0]=_vertices[1]-_vertices[0];
	_edge[1]=_vertices[2]-_vertices[0];
}

bool Triangle::RayIntersect(const CVector3D& origin,const CVector3D& dir,float& dist) const
{
    // begin calculating determinant - also used to calculate U parameter
    CVector3D pvec=dir.Cross(_edge[1]);

    // if determinant is near zero, ray lies in plane of triangle
    float det = _edge[0].Dot(pvec);
    if (fabs(det)<EPSILON)
        return false;

    float inv_det = 1.0f/det;

    // calculate vector from vert0 to ray origin
    CVector3D tvec;
	for (int i=0;i<3;++i) {
		tvec[i]=origin[i]-_vertices[0][i];
	}

    // calculate U parameter, test bounds
    float u=tvec.Dot(pvec)*inv_det;
    if (u<-0.01f || u>1.01f)
        return false;

    // prepare to test V parameter
    CVector3D qvec=tvec.Cross(_edge[0]);

    // calculate V parameter and test bounds
    float v=dir.Dot(qvec)*inv_det;
    if (v<-0.01f || u+v>1.01f)
        return false;

    // calculate distance to intersection point from ray origin
    float d=_edge[1].Dot(qvec)*inv_det;
    if (d>=0 && d<dist) {
        dist=d;
        return true;
    }

    return false;
}
