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

AndConstraint::AndConstraint(const vector<Constraint*>& constraints) {
	this->constraints = constraints;
}

AndConstraint::~AndConstraint() {
	for(int i=0; i<constraints.size(); i++) {
		delete constraints[i];
	}
}

bool AndConstraint::allows(Map* m, int x, int y)
{
	for(int i=0; i<constraints.size(); i++) {
		if(!constraints[i]->allows(m, x, y)) {
			return false;
		}
	}
	return true;
}