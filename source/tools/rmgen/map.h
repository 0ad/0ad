#ifndef __MAP_H__
#define __MAP_H__

#include "area.h"
#include "areapainter.h"
#include "areaplacer.h"
#include "constraint.h"
#include "entity.h"

class Map {
public:
	int size;
	int** terrain;
	float** height;
	Area*** area;
	std::map<std::string, int> nameToId;
	std::map<int, std::string> idToName;
	std::vector<Entity*> entities;
	std::vector<Area*> areas;

	Map(int size, const std::string& baseTerrain, float baseHeight);
	~Map();

	int getId(std::string terrain);

	bool validT(int x, int y);
	bool validH(int x, int y);

	std::string getTerrain(int x, int y);
	void setTerrain(int x, int y, const std::string& terrain);

	float getHeight(int x, int y);
	void setHeight(int x, int y, float height);

	void addEntity(class Entity* ent);

	Area* createArea(AreaPlacer* placer, AreaPainter* painter, Constraint* constr);
};

#endif