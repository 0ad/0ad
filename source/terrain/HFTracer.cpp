///////////////////////////////////////////////////////////////////////////////
//
// Name:		HFTracer.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "HFTracer.h"
#include "terrain/Terrain.h"
#include "terrain/Bound.h"
#include "terrain/Vector3D.h"

extern CTerrain g_Terrain;

///////////////////////////////////////////////////////////////////////////////
// CHFTracer constructor
CHFTracer::CHFTracer(const u16* hf,u32 mapsize,float cellsize,float heightscale)
	: m_Heightfield(hf), m_MapSize(mapsize), m_CellSize(cellsize), 
	m_HeightScale(heightscale)
{
} 


///////////////////////////////////////////////////////////////////////////////
// RayTriIntersect: intersect a ray with triangle defined by vertices 
// v0,v1,v2; return true if ray hits triangle at distance less than dist,
// or false otherwise
bool CHFTracer::RayTriIntersect(const CVector3D& v0,const CVector3D& v1,const CVector3D& v2,
								const CVector3D& origin,const CVector3D& dir,float& dist) const
{
	const float EPSILON=0.00001f;

	// calculate edge vectors
	CVector3D edge0=v1-v0;
	CVector3D edge1=v2-v0;

    // begin calculating determinant - also used to calculate U parameter
    CVector3D pvec=dir.Cross(edge1);

    // if determinant is near zero, ray lies in plane of triangle
    float det = edge0.Dot(pvec);
    if (fabs(det)<EPSILON)
        return false;

    float inv_det = 1.0f/det;

    // calculate vector from vert0 to ray origin
    CVector3D tvec=origin-v0;

    // calculate U parameter, test bounds
    float u=tvec.Dot(pvec)*inv_det;
    if (u<-0.01f || u>1.01f)
        return false;

    // prepare to test V parameter
    CVector3D qvec=tvec.Cross(edge0);

    // calculate V parameter and test bounds
    float v=dir.Dot(qvec)*inv_det;
    if (v<0.0f || u+v>1.0f)
        return false;

    // calculate distance to intersection point from ray origin
    float d=edge1.Dot(qvec)*inv_det;
    if (d>=0 && d<dist) {
        dist=d;
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// CellIntersect: test if ray intersects either of the triangles in the given 
// cell - return hit result, and distance to hit, if hit occurred
bool CHFTracer::CellIntersect(int cx,int cz,CVector3D& origin,CVector3D& dir,float& dist) const
{
	bool res=false;

	// get vertices for this cell
	CVector3D vpos[4];
	g_Terrain.CalcPosition(cx,cz,vpos[0]);
	g_Terrain.CalcPosition(cx+1,cz,vpos[1]);
	g_Terrain.CalcPosition(cx+1,cz+1,vpos[2]);
	g_Terrain.CalcPosition(cx,cz+1,vpos[3]);

	dist=1.0e30f;
	if (RayTriIntersect(vpos[0],vpos[1],vpos[2],origin,dir,dist)) {
		res=true;
	}

	if (RayTriIntersect(vpos[0],vpos[2],vpos[3],origin,dir,dist)) {
		res=true;
	}	

	return res;
}

///////////////////////////////////////////////////////////////////////////////
// RayIntersect: intersect ray with this heightfield; return true if 
// intersection occurs (and fill in grid coordinates of intersection), or false 
// otherwise
bool CHFTracer::RayIntersect(CVector3D& origin,CVector3D& dir,int& x,int& z,CVector3D& ipt) const
{
	// intersect first against bounding box
	CBound bound;
	bound[0]=CVector3D(0,0,0);
	bound[1]=CVector3D(m_MapSize*m_CellSize,65535*m_HeightScale,m_MapSize*m_CellSize);

	float tmin,tmax;
	if (!bound.RayIntersect(origin,dir,tmin,tmax)) {
		// ray missed world bounds; no intersection
		return false;
	}

	// project origin onto grid, if necessary, to get starting point for traversal
	CVector3D traversalPt;
	if (tmin>0) {
		traversalPt=origin+dir*tmin;
	} else {
		traversalPt=origin;
	}

	// setup traversal variables
	int sx=dir.X<0 ? -1 : 1;
	int sz=dir.Z<0 ? -1 : 1;

	float invCellSize=1.0f/float(m_CellSize);

	float fcx=traversalPt.X*invCellSize;
	int cx=int(fcx);

	float fcz=traversalPt.Z*invCellSize;
	int cz=int(fcz);

	float invdx=float(1.0/fabs(dir.X));
	float invdz=float(1.0/fabs(dir.Z));

	float dist;
	do {		
		// test current cell
		if (cx>=0 && cx<int(m_MapSize-1) && cz>=0 && cz<int(m_MapSize-1)) {
			if (CellIntersect(cx,cz,origin,dir,dist)) {
				x=cx;
				z=cz;
				ipt=origin+dir*dist;
				return true;
			}
		} 

		// get coords of current cell
		fcx=traversalPt.X*invCellSize;
		fcz=traversalPt.Z*invCellSize;

		// get distance to next cell in x,z
		float dx=(sx==-1) ? fcx-float(cx) : 1-(fcx-float(cx));
		dx*=invdx;
		float dz=(sz==-1) ? fcz-float(cz) : 1-(fcz-float(cz));
		dz*=invdz;

		// advance ..
		float dist;
		if (dx<dz) {
			cx+=sx;
			dist=dx;
		} else {
			cz+=sz;
			dist=dz;
		}

		traversalPt+=dir*dist;
	} while (traversalPt.Y>=0);
	
	// fell off end of heightmap with no intersection; return a miss
	return false;
}
