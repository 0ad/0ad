#ifndef __TILECLASS_H__
#define __TILECLASS_H__

#include "point.h"
#include "rangecount.h"

// Represents a multiset of tiles, used for constraints. That is, each tile 
// can be in the class one or more times. Provides operations to add/remove a tile
// (if a tile is added N times it has to be removed N times to be cleared), and to
// efficiently find whether any tiles in the class are within distance D of a point 
// (this will take O(D log D) time).
class TileClass {
private:
	int mapSize;
	std::vector<RangeCount*> rc;	// range count on each row
	int** inclusionCount;			// the inclusion count for each tile
public:
	TileClass(int mapSize);
	~TileClass();

	void add(int x, int y);
	void remove(int x, int y);

	void countInRadius(float cx, float cy, float r, int& members, int& nonMembers);
	int countMembersInRadius(float cx, float cy, float r);
	int countNonMembersInRadius(float cx, float cy, float r);
};

#endif