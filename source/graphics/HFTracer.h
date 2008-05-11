/**
 * =========================================================================
 * File        : HFTracer.h
 * Project     : 0 A.D.
 * Description : Determine intersection of rays with a heightfield.
 * =========================================================================
 */

#ifndef INCLUDED_HFTRACER
#define INCLUDED_HFTRACER

class CVector3D;
class CTerrain;

///////////////////////////////////////////////////////////////////////////////
// CHFTracer: a class for determining ray intersections with a heightfield
class CHFTracer
{
public:
	// constructor; setup data
	CHFTracer(CTerrain *pTerrain);

	// intersect ray with this heightfield; return true if intersection 
	// occurs (and fill in grid coordinates and point of intersection), or false otherwise
	bool RayIntersect(CVector3D& origin,CVector3D& dir,int& x,int& z,CVector3D& ipt) const;

private:
	// intersect a ray with triangle defined by vertices 
	// v0,v1,v2; return true if ray hits triangle at distance less than dist,
	// or false otherwise
	bool RayTriIntersect(const CVector3D& v0,const CVector3D& v1,const CVector3D& v2,
			const CVector3D& origin,const CVector3D& dir,float& dist) const;

	// test if ray intersects either of the triangles in the given 
	bool CellIntersect(int cx,int cz,CVector3D& origin,CVector3D& dir,float& dist) const;
	
	// The terrain we're operating on
	CTerrain *m_pTerrain;
	// the heightfield were tracing
	const u16* m_Heightfield;
	// size of the heightfield
	size_t m_MapSize;
	// cell size - size of each cell in x and z
	float m_CellSize;
	// vertical scale - size of each cell in y
	float m_HeightScale;
};

#endif
