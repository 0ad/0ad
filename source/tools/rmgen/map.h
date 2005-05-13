#ifndef __MAP_H__
#define __MAP_H__

class Map {
public:
    int size;
    int** terrain;
    float** height;
    std::map<std::string, int> nameToId;
    std::map<int, std::string> idToName;

    Map(int size, const std::string& baseTerrain, float baseHeight);
    ~Map();

    int getId(std::string terrain);

    bool validT(int x, int y);
    bool validH(int x, int y);

    std::string getTerrain(int x, int y);
    void setTerrain(int x, int y, const std::string& terrain);

    float getHeight(int x, int y);
    void setHeight(int x, int y, float height);
};

#endif