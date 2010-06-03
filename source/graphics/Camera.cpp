/* Copyright (C) 2010 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * CCamera holds a view and a projection matrix. It also has a frustum
 * which can be used to cull objects for rendering.
 */

#include "precompiled.h"

#include "Camera.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "HFTracer.h"
#include "ps/Game.h"
#include "lib/ogl.h"
#include "ps/World.h"

CCamera::CCamera ()
{
	// set viewport to something anything should handle, but should be initialised
	// to window size before use
	m_ViewPort.m_X = 0;
	m_ViewPort.m_Y = 0;
	m_ViewPort.m_Width = 800;
	m_ViewPort.m_Height = 600;
}

CCamera::~CCamera ()
{
}

void CCamera::SetProjection (float nearp, float farp, float fov)
{
	m_NearPlane = nearp;
	m_FarPlane = farp;
	m_FOV = fov;

	float Aspect = (float)m_ViewPort.m_Width/(float)m_ViewPort.m_Height;

	float w = tanf (m_FOV*0.5f*Aspect);
	float h = tanf (m_FOV*0.5f);

	m_ProjMat.SetZero ();
	m_ProjMat._11 = 1/w;
	m_ProjMat._22 = 1/h;
	m_ProjMat._33 = (m_FarPlane+m_NearPlane)/(m_FarPlane-m_NearPlane);
	m_ProjMat._34 = -2*m_FarPlane*m_NearPlane/(m_FarPlane-m_NearPlane);
	m_ProjMat._43 = 1.0f;
}

void CCamera::SetProjectionTile (int tiles, int tile_x, int tile_y)
{
	float Aspect = (float)m_ViewPort.m_Width/(float)m_ViewPort.m_Height;

	float w = tanf (m_FOV*0.5f*Aspect) / tiles;
	float h = tanf (m_FOV*0.5f) / tiles;

	m_ProjMat._11 = 1/w;
	m_ProjMat._22 = 1/h;
	m_ProjMat._13 = -(1-tiles + 2*tile_x);
	m_ProjMat._23 = -(1-tiles + 2*tile_y);
}

//Updates the frustum planes. Should be called
//everytime the view or projection matrices are
//altered.
void CCamera::UpdateFrustum ()
{
	CMatrix3D MatFinal;
	CMatrix3D MatView;

	m_Orientation.GetInverse(MatView);

	MatFinal = m_ProjMat * MatView;

	//get the RIGHT plane
	m_ViewFrustum.SetNumPlanes (6);

	m_ViewFrustum.m_aPlanes[0].m_Norm.X = MatFinal._41-MatFinal._11;
	m_ViewFrustum.m_aPlanes[0].m_Norm.Y = MatFinal._42-MatFinal._12;
	m_ViewFrustum.m_aPlanes[0].m_Norm.Z = MatFinal._43-MatFinal._13;
	m_ViewFrustum.m_aPlanes[0].m_Dist	= MatFinal._44-MatFinal._14;

	//get the LEFT plane
	m_ViewFrustum.m_aPlanes[1].m_Norm.X = MatFinal._41+MatFinal._11;
	m_ViewFrustum.m_aPlanes[1].m_Norm.Y = MatFinal._42+MatFinal._12;
	m_ViewFrustum.m_aPlanes[1].m_Norm.Z = MatFinal._43+MatFinal._13;
	m_ViewFrustum.m_aPlanes[1].m_Dist	= MatFinal._44+MatFinal._14;

	//get the BOTTOM plane
	m_ViewFrustum.m_aPlanes[2].m_Norm.X = MatFinal._41+MatFinal._21;
	m_ViewFrustum.m_aPlanes[2].m_Norm.Y = MatFinal._42+MatFinal._22;
	m_ViewFrustum.m_aPlanes[2].m_Norm.Z = MatFinal._43+MatFinal._23;
	m_ViewFrustum.m_aPlanes[2].m_Dist	= MatFinal._44+MatFinal._24;

	//get the TOP plane
	m_ViewFrustum.m_aPlanes[3].m_Norm.X = MatFinal._41-MatFinal._21;
	m_ViewFrustum.m_aPlanes[3].m_Norm.Y = MatFinal._42-MatFinal._22;
	m_ViewFrustum.m_aPlanes[3].m_Norm.Z = MatFinal._43-MatFinal._23;
	m_ViewFrustum.m_aPlanes[3].m_Dist	= MatFinal._44-MatFinal._24;

	//get the FAR plane
	m_ViewFrustum.m_aPlanes[4].m_Norm.X = MatFinal._41-MatFinal._31;
	m_ViewFrustum.m_aPlanes[4].m_Norm.Y = MatFinal._42-MatFinal._32;
	m_ViewFrustum.m_aPlanes[4].m_Norm.Z = MatFinal._43-MatFinal._33;
	m_ViewFrustum.m_aPlanes[4].m_Dist	= MatFinal._44-MatFinal._34;

	//get the NEAR plane
	m_ViewFrustum.m_aPlanes[5].m_Norm.X = MatFinal._41+MatFinal._31;
	m_ViewFrustum.m_aPlanes[5].m_Norm.Y = MatFinal._42+MatFinal._32;
	m_ViewFrustum.m_aPlanes[5].m_Norm.Z = MatFinal._43+MatFinal._33;
	m_ViewFrustum.m_aPlanes[5].m_Dist	= MatFinal._44+MatFinal._34;
}

void CCamera::SetViewPort (const SViewPort& viewport)
{
	m_ViewPort.m_X = viewport.m_X;
	m_ViewPort.m_Y = viewport.m_Y;
	m_ViewPort.m_Width = viewport.m_Width;
	m_ViewPort.m_Height = viewport.m_Height;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCameraPlanePoints: return four points in camera space at given distance from camera
void CCamera::GetCameraPlanePoints(float dist,CVector3D pts[4]) const
{
	float aspect=float(m_ViewPort.m_Width)/float(m_ViewPort.m_Height);

	float x=dist*float(tan(GetFOV()*aspect*0.5));
	float y=dist*float(tan(GetFOV()*0.5));
	pts[0].X=-x;
	pts[0].Y=-y;
	pts[0].Z=dist;
	pts[1].X=x;
	pts[1].Y=-y;
	pts[1].Z=dist;
	pts[2].X=x;
	pts[2].Y=y;
	pts[2].Z=dist;
	pts[3].X=-x;
	pts[3].Y=y;
	pts[3].Z=dist;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetFrustumPoints: calculate and return the position of the 8 points of the frustum in world space
void CCamera::GetFrustumPoints(CVector3D pts[8]) const
{
	// get camera space points for near and far planes
	CVector3D cpts[8];
	GetCameraPlanePoints(m_NearPlane,pts);
	GetCameraPlanePoints(m_FarPlane,pts+4);

	// transform to world space
	for (int i=0;i<8;i++) {
		m_Orientation.Transform(cpts[i],pts[i]);
	}
}

void CCamera::BuildCameraRay( int px, int py, CVector3D& origin, CVector3D& dir )
{
	CVector3D cPts[4];
	GetCameraPlanePoints( m_FarPlane, cPts );

	// transform to world space
	CVector3D wPts[4];
	for( int i = 0; i < 4; i++)
		wPts[i] = m_Orientation.Transform( cPts[i] );

	// get world space position of mouse point
	float dx = (float)px/(float)g_Renderer.GetWidth();
	float dz=1-(float)py/(float)g_Renderer.GetHeight();

	CVector3D vdx = wPts[1] - wPts[0];
	CVector3D vdz = wPts[3] - wPts[0];
	CVector3D pt = wPts[0] + ( vdx * dx ) + ( vdz * dz );

	// copy origin
	origin = m_Orientation.GetTranslation();
	// build direction
	dir = pt - origin;
	dir.Normalize();
}

void CCamera::GetScreenCoordinates( const CVector3D& world, float& x, float& y ) const
{
	CMatrix3D transform;
	m_Orientation.GetInverse( transform );
	transform.Concatenate( m_ProjMat );

	CVector3D screenspace = transform.Transform( world );

	x = screenspace.X / screenspace.Z;
	y = screenspace.Y / screenspace.Z;
	x = ( x + 1 ) * 0.5f * g_Renderer.GetWidth();
	y = ( 1 - y ) * 0.5f * g_Renderer.GetHeight();
}

CVector3D CCamera::GetWorldCoordinates( int px, int py, bool aboveWater )
{
	CHFTracer tracer( g_Game->GetWorld()->GetTerrain() );
	int x, z;
	CVector3D origin, dir, delta, terrainPoint, waterPoint;

	BuildCameraRay( px, py, origin, dir );


	bool gotTerrain = tracer.RayIntersect( origin, dir, x, z, terrainPoint );

	if( !aboveWater )
	{
		if( gotTerrain ) 
			return terrainPoint;

		// Off the edge of the world?
		// Work out where it /would/ hit, if the map were extended out to infinity with average height.
		return GetWorldCoordinates( px, py, 50.0f );
	}

	CPlane plane;
	plane.Set(CVector3D(0.f, 1.f, 0.f),										// upwards normal
		CVector3D(0.f, g_Renderer.GetWaterManager()->m_WaterHeight, 0.f));	// passes through water plane

	bool gotWater = plane.FindRayIntersection( origin, dir, &waterPoint );

	if( gotTerrain )
	{
		if( gotWater )
		{
			// Intersecting both heightmap and water plane; choose the closest of those
			if( (origin - terrainPoint).LengthSquared() < (origin - waterPoint).LengthSquared() )
				return terrainPoint;
			else
				return waterPoint;
		}
		else
		{
			// Intersecting heightmap but parallel to water plane
			return terrainPoint;
		}
	}
	else
	{
		if( gotWater )
		{
			// Only intersecting water plane
			return waterPoint;
		}
		else
		{
			// Not intersecting terrain or water; just return 0,0,0.
			return CVector3D(0.f, 0.f, 0.f);
		}
	}

}

CVector3D CCamera::GetWorldCoordinates(int px, int py, float h)
{
	CPlane plane;
	plane.Set(CVector3D(0.f, 1.f, 0.f), CVector3D(0.f, h, 0.f)); // upwards normal, passes through h

	CVector3D origin, dir, delta, currentTarget;

	BuildCameraRay(px, py, origin, dir);

	if (plane.FindRayIntersection(origin, dir, &currentTarget))
		return currentTarget;

	// No intersection with the infinite plane - nothing sensible can be returned,
	// so just choose an arbitrary point on the plane
	return CVector3D(0.f, h, 0.f);
}

CVector3D CCamera::GetFocus()
{
	// Basically the same as GetWorldCoordinates

	CHFTracer tracer( g_Game->GetWorld()->GetTerrain() ); int x, z;

	CVector3D origin, dir, delta, currentTarget;

	origin = m_Orientation.GetTranslation();
	dir = m_Orientation.GetIn();

	if( tracer.RayIntersect( origin, dir, x, z, currentTarget ) )
		return( currentTarget );

	// Off the edge of the world?
	// Work out where it /would/ hit, if the map were extended out to infinity with average height.

	return( origin + dir * ( ( 50.0f - origin.Y ) / dir.Y ) );
}

void CCamera::LookAt( const CVector3D& camera, const CVector3D& target, const CVector3D& up )
{
	CVector3D delta = target - camera;
	LookAlong( camera, delta, up );
}

void CCamera::LookAlong( CVector3D camera, CVector3D orientation, CVector3D up )
{
	orientation.Normalize();
	up.Normalize();
	CVector3D s = orientation.Cross( up );

	m_Orientation._11 = -s.X;	m_Orientation._12 = up.X;	m_Orientation._13 = orientation.X;	m_Orientation._14 = camera.X;
	m_Orientation._21 = -s.Y;	m_Orientation._22 = up.Y;	m_Orientation._23 = orientation.Y;	m_Orientation._24 = camera.Y;
	m_Orientation._31 = -s.Z;	m_Orientation._32 = up.Z;	m_Orientation._33 = orientation.Z;	m_Orientation._34 = camera.Z;
	m_Orientation._41 = 0.0f;	m_Orientation._42 = 0.0f;	m_Orientation._43 = 0.0f;			m_Orientation._44 = 1.0f;
}


///////////////////////////////////////////////////////////////////////////////////
// Render the camera's frustum
void CCamera::Render(int intermediates) const
{
	CVector3D nearPoints[4];
	CVector3D farPoints[4];

	GetCameraPlanePoints(m_NearPlane, nearPoints);
	GetCameraPlanePoints(m_FarPlane, farPoints);
	for(int i = 0; i < 4; i++)
	{
		nearPoints[i] = m_Orientation.Transform(nearPoints[i]);
		farPoints[i] = m_Orientation.Transform(farPoints[i]);
	}

	// near plane
	glBegin(GL_POLYGON);
		glVertex3fv(&nearPoints[0].X);
		glVertex3fv(&nearPoints[1].X);
		glVertex3fv(&nearPoints[2].X);
		glVertex3fv(&nearPoints[3].X);
	glEnd();

	// far plane
	glBegin(GL_POLYGON);
		glVertex3fv(&farPoints[0].X);
		glVertex3fv(&farPoints[1].X);
		glVertex3fv(&farPoints[2].X);
		glVertex3fv(&farPoints[3].X);
	glEnd();

	// connection lines
	glBegin(GL_QUAD_STRIP);
		glVertex3fv(&nearPoints[0].X);
		glVertex3fv(&farPoints[0].X);
		glVertex3fv(&nearPoints[1].X);
		glVertex3fv(&farPoints[1].X);
		glVertex3fv(&nearPoints[2].X);
		glVertex3fv(&farPoints[2].X);
		glVertex3fv(&nearPoints[3].X);
		glVertex3fv(&farPoints[3].X);
		glVertex3fv(&nearPoints[0].X);
		glVertex3fv(&farPoints[0].X);
	glEnd();

	// intermediate planes
	CVector3D intermediatePoints[4];
	for(int i = 0; i < intermediates; ++i)
	{
		float t = (i+1.0)/(intermediates+1.0);

		for(int j = 0; j < 4; ++j)
			intermediatePoints[j] = nearPoints[j]*t + farPoints[j]*(1.0-t);

		glBegin(GL_POLYGON);
			glVertex3fv(&intermediatePoints[0].X);
			glVertex3fv(&intermediatePoints[1].X);
			glVertex3fv(&intermediatePoints[2].X);
			glVertex3fv(&intermediatePoints[3].X);
		glEnd();
	}
}


