#ifndef __SIMPLECONSTRAINTS_H__
#define __SIMPLECONSTRAINTS_H__

#include "constraint.h"
#include "map.h"
#include "area.h"

class NullConstraint : public Constraint {
public:
	virtual bool allows(Map* m, int x, int y);
};

class AvoidAreaConstraint : public Constraint {
private:
	Area* area;
public:
	AvoidAreaConstraint(Area* area);
	virtual bool allows(Map* m, int x, int y);
};

class AvoidTextureConstraint : public Constraint {
private:
	int textureId;
public:
	AvoidTextureConstraint(int textureId);
	virtual bool allows(Map* m, int x, int y);
};

class AndConstraint : public Constraint {
private:
	Constraint* a;
	Constraint* b;
public:
	AndConstraint(Constraint* a, Constraint* b);
	virtual bool allows(Map* m, int x, int y);
};

#endif
