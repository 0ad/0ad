#ifndef __TERRAIN_H__
#define __TERRAIN_H__

class Terrain
{
public:
	Terrain();
	void place(class Map* m, int x, int y);
	virtual void placeNew(class Map* m, int x, int y) = 0;	// template method
	virtual ~Terrain(void);
};

class SimpleTerrain: public Terrain
{
private:
	std::string texture;
	std::string treeType;

public:
	SimpleTerrain(const std::string& texture);
	SimpleTerrain(const std::string& texture, const std::string& treeType);

	static Terrain* parse(const std::string& name);

	void placeNew(class Map* m, int x, int y);
	~SimpleTerrain(void);
};

class RandomTerrain: public Terrain
{
private:
	std::vector<Terrain*> terrains;

public:
	RandomTerrain(const std::vector<Terrain*>& terrains);

	void placeNew(class Map* m, int x, int y);
	~RandomTerrain(void);
};


extern std::map<std::string, Terrain*> parsedTerrains;

#endif