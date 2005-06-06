#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "entity.h"

using namespace std;

Map::Map(int size, Terrain* baseTerrain, float baseHeight) {
	if(size<0 || size>1024) {
		JS_ReportError(cx, "init: map size out of range");
	}
	else if(size%16 != 0) {
		JS_ReportError(cx, "init: map size must be divisible by 16");
	}

	this->size = size;

	texture = new int*[size];
	for(int i=0; i<size; i++) {
		texture[i] = new int[size];
	}

	terrainEntities = new vector<Entity*>*[size];
	for(int i=0; i<size; i++) {
		terrainEntities[i] = new vector<Entity*>[size];
	}

	area = new Area**[size];
	for(int i=0; i<size; i++) {
		area[i] = new Area*[size];
		for(int j=0; j<size; j++) {
			area[i][j] = 0;
		}
	}

	height = new float*[size+1];
	for(int i=0; i<size+1; i++) {
		height[i] = new float[size+1];
		for(int j=0; j<size+1; j++) {
			height[i][j] = baseHeight;
		}
	}

	for(int i=0; i<size; i++) {
		for(int j=0; j<size; j++) {
			baseTerrain->place(this, i, j);
		}
	}
}

Map::~Map() {
	for(int i=0; i<size; i++) {
		delete[] texture[i];
	}
	delete[] texture;

	for(int i=0; i<size; i++) {
		delete[] terrainEntities[i];
	}
	delete[] terrainEntities;

	for(int i=0; i<size+1; i++) {
		delete[] height[i];
	}
	delete[] height;

	for(int i=0; i<size; i++) {
		delete[] area[i];
	}
	delete[] area;

	for(int i=0; i<entities.size(); i++) {
		delete entities[i];
	}
}

int Map::getId(string texture) {
	if(nameToId.find(texture) != nameToId.end()) {
		return nameToId[texture];
	}
	else {
		int newId = nameToId.size();
		nameToId[texture] = newId;
		idToName[newId] = texture;
		return newId;
	}
}

bool Map::validT(int x, int y) {
	return x>=0 && y>=0 && x<size && y<size;
}

bool Map::validH(int x, int y) {
	return x>=0 && y>=0 && x<size+1 && y<size+1;
}

string Map::getTexture(int x, int y) {
	if(!validT(x,y)) JS_ReportError(cx, "getTexture: invalid tile position");
	return idToName[texture[x][y]];
}

void Map::setTexture(int x, int y, const string& t) {
	if(!validT(x,y)) JS_ReportError(cx, "setTexture: invalid tile position");
	texture[x][y] = getId(t);
}

float Map::getHeight(int x, int y) {
	if(!validH(x,y)) JS_ReportError(cx, "getHeight: invalid point position");
	return height[x][y];
}

void Map::setHeight(int x, int y, float h) {
	if(!validH(x,y)) JS_ReportError(cx, "setHeight: invalid point position");
	height[x][y] = h;
}

void Map::placeTerrain(int x, int y, Terrain* t) {
	t->place(this, x, y);
}

void Map::addEntity(Entity* ent) {
	entities.push_back(ent);
}

Area* Map::createArea(AreaPlacer* placer, AreaPainter* painter, Constraint* constr) {
	vector<Point> points;
	if(!placer->place(this, constr, points)) {
		return 0;
	}
	Area* a = new Area(points);
	for(int i=0; i<points.size(); i++) {
		area[points[i].x][points[i].y] = a;
	}
	painter->paint(this, a);
	areas.push_back(a);
	return a;
}