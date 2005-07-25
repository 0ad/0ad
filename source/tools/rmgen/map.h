#ifndef __MAP_H__
#define __MAP_H__

#include "area.h"
#include "areapainter.h"
#include "areaplacer.h"
#include "constraint.h"
#include "entity.h"
#include "terrain.h"
#include "objectgroupplacer.h"

class Map {
public:
	int size;
	int** texture;
	std::vector<Entity*>** terrainEntities;
	float** height;
	Area*** area;
	std::map<std::string, int> nameToId;
	std::map<int, std::string> idToName;
	std::vector<Entity*> entities;
	std::vector<Area*> areas;

	Map(int size, Terrain* baseTerrain, float baseHeight);
	Map(std::string fileName, int loadLevel);
	~Map();

	int getId(std::string texture);

	bool validT(int x, int y);
	bool validH(int x, int y);

	std::string getTexture(int x, int y);
	void setTexture(int x, int y, const std::string& texture);

	float getHeight(int x, int y);
	void setHeight(int x, int y, float height);

	std::vector<Entity*> getTerrainEntities(int x, int y);
	void setTerrainEntities(int x, int y, std::vector<Entity*> &entities);

	void placeTerrain(int x, int y, Terrain* t);

	void addEntity(class Entity* ent);

	Area* createArea(AreaPlacer* placer, AreaPainter* painter, Constraint* constr);
	std::vector<Entity*>* createObjectGroup(ObjectGroupPlacer* placer, Constraint* constr);
};

#endif