#include "stdafx.h"
#include "rmgen.h"
#include "map.h"

using namespace std;

Map::Map(int size, const string& baseTerrain, float baseHeight) {
    if(size<0 || size>1024) {
        JS_ReportError(cx, "init: map size out of range");
    }
    else if(size%16 != 0) {
        JS_ReportError(cx, "init: map size must be divisble by 16");
    }

    this->size = size;

    int baseId = getId(baseTerrain);

    terrain = new int*[size];
    for(int i=0; i<size; i++) {
        terrain[i] = new int[size];
        for(int j=0; j<size; j++) {
            terrain[i][j] = baseId;
        }
    }
    
    height = new float*[size+1];
    for(int i=0; i<size+1; i++) {
        height[i] = new float[size+1];
        for(int j=0; j<size+1; j++) {
            height[i][j] = baseHeight;
        }
    }
}

Map::~Map() {
    for(int i=0; i<size; i++) {
        delete[] terrain[i];
    }
    delete[] terrain;

    for(int i=0; i<size+1; i++) {
        delete[] height[i];
    }
    delete[] height;
}

int Map::getId(string terrain) {
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

bool Map::validT(int x, int y) {
    return x>=0 && y>=0 && x<size && y<size;
}

bool Map::validH(int x, int y) {
    return x>=0 && y>=0 && x<size+1 && y<size+1;
}

string Map::getTerrain(int x, int y) {
    if(!validT(x,y)) JS_ReportError(cx, "getTerrain: invalid tile position");
    else return idToName[terrain[x][y]];
}

void Map::setTerrain(int x, int y, const string& t) {
    if(!validT(x,y)) JS_ReportError(cx, "setTerrain: invalid tile position");
    else terrain[x][y] = getId(t);
}

float Map::getHeight(int x, int y) {
    if(!validH(x,y)) JS_ReportError(cx, "getHeight: invalid point position");
    else return height[x][y];
}

void Map::setHeight(int x, int y, float h) {
    if(!validH(x,y)) JS_ReportError(cx, "setHeight: invalid point position");
    else height[x][y] = h;
}