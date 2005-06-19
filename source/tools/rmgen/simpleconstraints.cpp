#include "stdafx.h"
#include "simpleconstraints.h"

using namespace std;

// NullConstraint

bool NullConstraint::allows(Map* m, int x, int y)
{
	return true;
}

// AvoidAreaConstraint

AvoidAreaConstraint::AvoidAreaConstraint(Area* area) {
	this->area = area;
}

bool AvoidAreaConstraint::allows(Map* m, int x, int y)
{
	return m->area[x][y] != area;
}

// AvoidTextureConstraint

AvoidTextureConstraint::AvoidTextureConstraint(int textureId) {
	this->textureId = textureId;
}

bool AvoidTextureConstraint::allows(Map* m, int x, int y)
{
	return m->texture[x][y] != textureId;
}

// AndConstraint

AndConstraint::AndConstraint(Constraint* a, Constraint*b) {
	this->a = a;
	this->b = b;
}

bool AndConstraint::allows(Map* m, int x, int y)
{
	return a->allows(m,x,y) && b->allows(m,x,y);
}