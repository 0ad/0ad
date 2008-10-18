#include "stdafx.h"
#include "rmgen.h"
#include "map.h"
#include "object.h"
#include "pmp_file.h"

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

	terrainObjects = new vector<Object*>*[size];
	for(int i=0; i<size; i++) {
		terrainObjects[i] = new vector<Object*>[size];
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

Map::Map(string fileName, int loadLevel)
{
	const int LOAD_NOTHING = 0;
	const int LOAD_TERRAIN = 1<<0;
	const int LOAD_INTERACTIVES = 1 << 1;
	const int LOAD_NONINTERACTIVES = 1 << 2;
	//const int SOMETHINGELSE = 1 << 3;


	// HACK, this should probably be in a struct and be shared with the code in output.cpp
	pmp_header hdr;
	u32 map_size;

	//HACK, also in rmgen.cpp
	const string SCENARIO_PATH = "../data/mods/public/maps/scenarios/";

	std::string pmpFile = SCENARIO_PATH + fileName + ".pmp";
	std::string xmlFile = SCENARIO_PATH + fileName + ".xml";

	if (loadLevel & LOAD_TERRAIN)
	{

		FILE* f = fopen(pmpFile.c_str(), "rb");

		fread(&hdr, sizeof(pmp_header), 1, f);

		/*TODO

		if (hdr.marker !== "PSMP")
			JS_ReportError(cx, "");

		if (hdr.version !== 4)
			JS_ReportError(cx, ""); */
		

		fread(&map_size, sizeof(int), 1, f);

		size = map_size * 16;

		// Load height data
		u16* heightmap = new u16[(size+1) * (size+1)];
		fread(heightmap, 2, ((size+1)*(size+1)), f);

		height = new float*[(size+1)];
		for(int i=0; i<(size+1); i++) 
		{
			height[i] = new float[(size+1)];
			for(int j=0; j<(size+1); j++) 
			{
				height[i][j] = (float) (heightmap[(j*(size+1)) + i] / 256.0f) * 0.35f;
			}
		}


		// Load the list of used textures

		int numTextures;
		int strLength;

		fread(&numTextures, sizeof(int), 1, f);

		for (int i=0; i<numTextures; i++)
		{
			fread(&strLength, sizeof(int), 1, f);
			std::string name;

			vector<char> buf(strLength+1);
			fread(&buf[0], 1, strLength, f);
			name = &buf[0];

			// This will add the texture to the NameToId and idToName vectors if it 
			// doesn't already exist. And in this case it shouldn't. 
			getId( name.substr(0, name.length()-4));
		}

		texture = new int*[size];
		for(int i=0; i<size; i++) {
			texture[i] = new int[size];
		}

		Tile* tiles = new Tile[size*size];
		fread(tiles, sizeof(Tile), size*size, f);

		for(int x=0; x<size; x++) 
		{
			for(int y=0; y<size; y++) 
			{
				int patchX = x/16, patchY = y/16;
				int offX = x%16, offY = y%16;
				Tile& t = tiles[ (patchY*size/16 + patchX)*16*16 + (offY*16 + offX) ];
				this->texture[x][y] = t.texture1;
			}
		}
		
		terrainObjects = new vector<Object*>*[size];
		for(int i=0; i<size; i++) {
			terrainObjects[i] = new vector<Object*>[size];
		}

		area = new Area**[size];
		for(int i=0; i<size; i++) {
			area[i] = new Area*[size];
			for(int j=0; j<size; j++) {
				area[i][j] = 0;
			}
		}

		fclose(f);
	}


	if ((loadLevel & (LOAD_INTERACTIVES | LOAD_NONINTERACTIVES)) != LOAD_NOTHING)
	{

		//TODO: Load xml here

		if (loadLevel & LOAD_INTERACTIVES)
		{
			//printf("Loading interactives..\n");
		}


		if (loadLevel & LOAD_NONINTERACTIVES)
		{
			//printf("Loading non-interactives..\n");
		}

	}


}

Map::~Map() {
	for(int i=0; i<size; i++) {
		delete[] texture[i];
	}
	delete[] texture;

	for(int i=0; i<size; i++) {
		delete[] terrainObjects[i];
	}
	delete[] terrainObjects;

	for(int i=0; i<size+1; i++) {
		delete[] height[i];
	}
	delete[] height;

	for(int i=0; i<size; i++) {
		delete[] area[i];
	}
	delete[] area;

	for(int i=0; i<objects.size(); i++) {
		delete objects[i];
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

string Map::getTexture(int x, int y) {
	if(!validT(x,y)) JS_ReportError(cx, "getTexture: invalid tile position");
	return idToName[texture[x][y]];
}

void Map::setTexture(int x, int y, const string& t) {
	if(!validT(x,y)) JS_ReportError(cx, "setTexture: invalid tile position");
	texture[x][y] = getId(t);
}

float Map::getHeight(int x, int y) {
	if(!validH(x,y)) JS_ReportError(cx, "getHeight: invalid vertex position");
	return height[x][y];
}

void Map::setHeight(int x, int y, float h) {
	if(!validH(x,y)) JS_ReportError(cx, "setHeight: invalid vertex position");
	height[x][y] = h;
}

vector<Object*> Map::getTerrainObjects(int x, int y) {
	if(!validT(x,y)) JS_ReportError(cx, "getTerrainObjects: invalid tile position");
	return terrainObjects[x][y];
}

void Map::setTerrainObjects(int x, int y, vector<Object*> &objects) {
	if(!validT(x,y)) JS_ReportError(cx, "setTerrainObjects: invalid tile position");
	terrainObjects[x][y] = objects;
}

void Map::placeTerrain(int x, int y, Terrain* t) {
	t->place(this, x, y);
}

void Map::addObject(Object* ent) {
	objects.push_back(ent);
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

bool Map::createObjectGroup(ObjectGroupPlacer* placer, int player, Constraint* constr) {
	return placer->place(this, player, constr);
}

int Map::createTileClass() {
	tileClasses.push_back(new TileClass(size));
	return tileClasses.size();
}

float Map::getExactHeight(float x, float y) {
	// copied & modified from ScEd

	int xi = min((int) floor(x), size);
	int yi = min((int) floor(y), size);
	float xf = x - xi;
	float yf = y - yi;

	float h00 = height[xi][yi];
	float h01 = height[xi][yi+1];
	float h10 = height[xi+1][yi];
	float h11 = height[xi+1][yi+1];

	return ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ;
}
