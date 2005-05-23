#ifndef __ENTITY_H__
#define __ENTITY_H__

class Entity {
public:
    std::string type;   // called "template" in XML?
    int player;
    float x, y, z;
    float orientation;

    Entity();
    Entity(const std::string& type, int player, float x, float y, float z, float orientation);
};

#endif