#include "stdafx.h"
#include "tileclass.h"

using namespace std;

TileClass::TileClass(int mapSize) {
	this->mapSize = mapSize;
	inclusionCount = new int*[mapSize];
	for(int i=0; i<mapSize; i++) {
		inclusionCount[i] = new int[mapSize];
		memset(inclusionCount[i], 0, mapSize*sizeof(int));
		rc.push_back(new RangeCount(mapSize));
	}
}

TileClass::~TileClass() {
	for(int i=0; i<mapSize; i++) {
		delete inclusionCount[i];
		delete rc[i];
	}
	delete inclusionCount;
}

void TileClass::add(int x, int y) {
	if(!inclusionCount[x][y]) {
		rc[y]->add(x,1);
	}
	inclusionCount[x][y]++;
}

void TileClass::remove(int x, int y) {
	inclusionCount[x][y]--;
	if(!inclusionCount[x][y]) {
		rc[y]->add(x, -1);
	}
}

int TileClass::countMembersInRadius(float cx, float cy, float r) {
	int mem, nonMem;
	countInRadius(cx, cy, r, mem, nonMem);
	return mem;
}

int TileClass::countNonMembersInRadius(float cx, float cy, float r) {
	int mem, nonMem;
	countInRadius(cx, cy, r, mem, nonMem);
	return nonMem;
}

void TileClass::countInRadius(float cx, float cy, float r, int& members, int& nonMembers) {
	members = 0;
	nonMembers = 0;

	for(float y = cy-r; y <= cy+r; y++) {
		int iy = (int) y;
		if(iy < 0 || iy >= mapSize) {
			continue;
		}

		float dy = y - cy;
		float dx = sqrt(r*r - dy*dy);
		float x1 = cx - dx;
		float x2 = cx + dx;
		int minX = max(0, (int) x1);
		int maxX = min(mapSize-1, (int) x2);
		int total = maxX - minX + 1;
		int mem = rc[iy]->get(minX, maxX+1);
		members += mem;
		nonMembers += total - mem;
	}
}