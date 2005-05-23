#include "stdafx.h"
#include "entity.h"

using namespace std;

Entity::Entity() {
}

Entity::Entity(const string& type, int player, float x, float y, float z, float orientation) {
    this->type = type;
    this->player = player;
    this->x = x;
    this->y = y;
    this->z = z;
    this->orientation = orientation;
}