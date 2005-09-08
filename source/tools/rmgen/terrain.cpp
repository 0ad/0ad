#include "stdafx.h"
#include "terrain.h"
#include "map.h"
#include "object.h"
#include "random.h"
#include "rmgen.h"

using namespace std;

// Terrain

Terrain::Terrain() {}

Terrain::~Terrain() {}

void Terrain::place(Map* m, int x, int y) {
	vector<Object*>& vec = m->terrainObjects[x][y];
	for(int i=0; i<vec.size(); i++) {
		delete vec[i];
	}
	vec.clear();
	placeNew(m, x, y);
}

// SimpleTerrain

SimpleTerrain::SimpleTerrain(const std::string& texture)
{
	this->texture = texture;
	this->treeType = "";
}

SimpleTerrain::SimpleTerrain(const std::string& texture, const std::string& treeType)
{
	this->texture = texture;
	this->treeType = treeType;
}

void SimpleTerrain::placeNew(Map* m, int x, int y) {
	vector<Object*>& vec = m->terrainObjects[x][y];
	if(treeType != "") {
		vec.push_back(new Object(treeType, 0, x+0.5f, y+0.5f, RandFloat()*PI));
	}
	m->texture[x][y] = m->getId(texture);
}

SimpleTerrain::~SimpleTerrain(void)
{
}

Terrain* SimpleTerrain::parse(const string& name) {
	static map<string, Terrain*> parsedTerrains;

	if(parsedTerrains.find(name) != parsedTerrains.end()) {
		return parsedTerrains[name];
	}
	else {
		string texture, treeType;
		size_t pos = name.find('|');
		if(pos != name.npos) {
			texture = name.substr(0, pos);
			treeType = name.substr(pos+1, name.size()-pos-1);
		}
		else {
			texture = name;
			treeType = "";
		}
		return parsedTerrains[name] = new SimpleTerrain(texture, treeType);
	}
}

// RandomTerrain

RandomTerrain::RandomTerrain(const vector<Terrain*>& terrains)
{
	if(terrains.size()==0) {
		JS_ReportError(cx, "RandomTerrain: terrains array must not be empty");
	}
	this->terrains = terrains;
}

void RandomTerrain::placeNew(Map* m, int x, int y) {
	terrains[RandInt(terrains.size())]->placeNew(m, x, y);
}

RandomTerrain::~RandomTerrain(void)
{
}
