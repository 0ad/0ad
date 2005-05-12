#pragma once

typedef int TerrainId;

class Map {
public:
	int size;
	TerrainId** terrain;
	std::map<std::string, int> nameToId;
	std::map<int, std::string> idToName;

	Map(int size, std::string baseTerrain);
	~Map();

	TerrainId getId(std::string terrain);
};