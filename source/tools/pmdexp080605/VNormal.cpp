#include "MaxInc.h"
#include "VNormal.h"



void VNormal::add(CVector3D& n,unsigned int s)
{
	if (!(s&smooth) && init) {
		if (next) {
			next->add(n,s);
		} else {
			next=new VNormal(n,s);
		}
	} else {
		_normal+=n;
		smooth|=s;
		init=true;
	}
}

VNormal* VNormal::get(unsigned int s)
{
	if (smooth&s || !next) {
		return this;
	} else {
		return next->get(s);
	}
}

void VNormal::get(unsigned int s,CVector3D& normal)
{
	if (smooth&s || !next) {
		normal=_normal;
	} else {
		next->get(s,normal);	
	}
}

void VNormal::normalize()
{
	VNormal *ptr = next, *prev = this;
	while (ptr) {
		if (ptr->smooth&smooth) {
			_normal += ptr->_normal;			
			prev->next = ptr->next;
			delete ptr;
			ptr = prev->next;
		} else {
			prev = ptr;
			ptr  = ptr->next;
		}
	}
	
	_normal.Normalize();
	if (next) next->normalize();
}
