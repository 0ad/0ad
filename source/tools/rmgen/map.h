#ifndef __MAP_H__
#define __MAP_H__

#include "area.h"
#include "areapainter.h"
#include "areaplacer.h"
#include "constraint.h"
#include "object.h"
#include "terrain.h"
#include "objectgroupplacer.h"
#include "tileclass.h"

class Map {
public:
	int size;
	int** texture;
	std::vector<Object*>** terrainObjects;
	float** height;
	Area*** area;
	std::map<std::string, int> nameToId;
	std::map<int, std::string> idToName;
	std::vector<Object*> objects;
	std::vector<Area*> areas;
	std::vector<TileClass*> tileClasses;

	Map(int size, Terrain* baseTerrain, float baseHeight);
	Map(std::string fileName, int loadLevel);
	~Map();

	int getId(std::string texture);

	bool validT(int x, int y) { return x>=0 && y>=0 && x<size && y<size; }
	bool validH(int x, int y) { return x>=0 && y>=0 && x<=size && y<=size; }

	bool validClass(int c) { return c>=0 && c<(int)tileClasses.size(); }

	std::string getTexture(int x, int y);
	void setTexture(int x, int y, const std::string& texture);

	float getHeight(int x, int y);
	void setHeight(int x, int y, float height);

	std::vector<Object*> getTerrainObjects(int x, int y);
	void setTerrainObjects(int x, int y, std::vector<Object*> &objects);

	void placeTerrain(int x, int y, Terrain* t);

	void addObject(class Object* ent);

	Area* createArea(AreaPlacer* placer, AreaPainter* painter, Constraint* constr);
	bool createObjectGroup(ObjectGroupPlacer* placer, int player, Constraint* constr);

	int createTileClass();	// returns ID of the new class
	
	float getExactHeight(float x, float y);		// get height taking into account terrain curvature
};

#endif