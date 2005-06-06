#include "stdafx.h"
#include "simpleplacers.h"
#include "random.h"

using namespace std;

// ExactPlacer

ExactPlacer::ExactPlacer(CenteredPlacer* centeredPlacer, int x, int y) {
	this->centeredPlacer = centeredPlacer;
	this->x = x;
	this->y = y;
}

ExactPlacer::~ExactPlacer() {
}

bool ExactPlacer::place(Map* m, Constraint* constr, std::vector<Point>& ret) {
	return centeredPlacer->place(m, constr, ret, x, y);
}

// MultiPlacer

MultiPlacer::MultiPlacer(CenteredPlacer* centeredPlacer, int numToPlace, int maxFail) {
	this->centeredPlacer = centeredPlacer;
	this->numToPlace = numToPlace;
	this->maxFail = maxFail;
}

MultiPlacer::~MultiPlacer() {
}

bool MultiPlacer::place(Map* m, Constraint* constr, std::vector<Point>& ret) {
	int failed = 0;
	int placed = 0;
	while(placed < numToPlace && failed <= maxFail) {
		int x = RandInt(m->size);
		int y = RandInt(m->size);
		if(constr->allows(m,x,y) && centeredPlacer->place(m, constr, ret, x, y)) {
			placed++;
		}
		else {
			failed++;
		}
	}
	return placed == numToPlace;
}