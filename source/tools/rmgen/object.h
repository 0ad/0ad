#ifndef __OBJECT_H__
#define __OBJECT_H__

class Object {
public:
	std::string name;	// "template" field for objects, "actor" field for nonobjects
	int player;			// -1 for nonobjects
	float x, y;
	float orientation;

	Object();
	Object(const std::string& name, int player, float x, float y, float orientation);
	bool isEntity();
};

#endif