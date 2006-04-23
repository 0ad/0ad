#include "stdafx.h"
#include "object.h"

using namespace std;

Object::Object() {
}

Object::Object(const string& name, int player, float x, float y, float orientation) {
	this->name = name;
	this->player = player;
	this->x = x;
	this->y = y;
	this->orientation = orientation;
}

bool Object::isEntity() {
	for(int i=0; i<name.size(); i++) {
		if(name[i]=='/') return false;
	}
	return true;
}