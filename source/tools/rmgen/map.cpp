#include "stdafx.h"
#include "rmgen.h"
#include "map.h"

using namespace std;

Map::Map(int size, string baseTerrain) {
	if(size<0 || size>1024) {
		JS_ReportError(cx, "init: map size out of range");
	}
	if(size%16 != 0) {
		JS_ReportError(cx, "init: map size must be divisble by 16");
	}

	this->size = size;

	int baseId = getId(baseTerrain);

	terrain = new TerrainId*[size];
	for(int i=0; i<size; i++) {
		terrain[i] = new TerrainId[size];
		for(int j=0; j<size; j++) {
			terrain[i][j] = baseId;
		}
	}
}

Map::~Map() {
	for(int i=0; i<size; i++) {
		delete[] terrain[i];
	}
	delete[] terrain;
}

TerrainId Map::getId(string terrain) {
	if(nameToId.find(terrain) != nameToId.end()) {
		return nameToId[terrain];
	}
	else {
		int newId = nameToId.size();
		nameToId[terrain] = newId;
		idToName[newId] = terrain;
		return newId;
	}
}