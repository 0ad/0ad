#ifndef __VNORMAL_H
#define __VNORMAL_H

#include "Vector3D.h"

class VNormal
{
public:
	CVector3D _normal;
	unsigned int smooth;
	VNormal *next;
	bool init;

	VNormal() {
		smooth=0;
		next=0;
		init=false;
		_normal=CVector3D(0,0,0);
	}

	VNormal(CVector3D& n,unsigned int s) {
		next=0;
		init=true;
		_normal=n;
		smooth=s;
	}

	~VNormal() {delete next;}
	void add(CVector3D &n,unsigned int s);
	VNormal* get(unsigned int s);
	void get(unsigned int s,CVector3D& normal);
	void normalize();
};


#endif