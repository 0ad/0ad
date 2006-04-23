//Desc:  Contains classes for smooth splines
//Borrowed from Game Programming Gems4.  (Slightly changed to better suit our purposes 
//(and compatability).  Any references to external material can be found there

#ifndef NUSPLINE_H
#define NUSPLINE_H

#define MAX_SPLINE_NODES 40
#include <stdlib.h>
#include "Vector3D.h"

struct SplineData
{
    CVector3D Position;
    CVector3D Velocity;
    float Distance;
};

class RNSpline
{
public:
	
	RNSpline() { NodeCount = 0; }
  void AddNode(const CVector3D &pos);
  void BuildSpline();
  CVector3D GetPosition(float time);

  std::vector<SplineData> Node;
  float MaxDistance;
  int NodeCount;

protected:
  CVector3D GetStartVelocity(int index);
  CVector3D GetEndVelocity(int index);
};

class SNSpline : public RNSpline
{
public:
  void BuildSpline(){ RNSpline::BuildSpline(); Smooth(); Smooth(); Smooth(); }
  void Smooth();
};

class TNSpline : public SNSpline
{
public:
  void AddNode(const CVector3D &pos, float timePeriod);
  void PushNode() { Node.push_back( SplineData() ); }
  void InsertNode(const int index, const CVector3D &pos, float timePeriod);
  void RemoveNode(const int index);

  void BuildSpline(){ RNSpline::BuildSpline(); for (int i=0;i<5;i++) { Smooth(); Smooth(); Smooth(); } }
  void Smooth(){ for( int x=0; x<3; x++ ) { SNSpline::Smooth(); Constrain(); }  }
  void Constrain(); 
};

#endif // NUSPLINE_H
