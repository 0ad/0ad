#ifndef SHAREDTYPES_H__
#define SHAREDTYPES_H__

#include "Shareable.h"

class wxPoint;
class CVector3D;

namespace AtlasMessage
{

// Represents a position in the game world, with an interface usable from the
// UI (which usually knows only about screen space). Typically constructed
// by the UI, then passed to the game which uses GetWorldSpace.

struct Position
{
	Position() : type(0) { type0.x = type0.y = type0.z = 0.f;  }

	// Constructs a position with specified world-space coordinates
	Position(float x_, float y_, float z_) : type(0) { type0.x = x_; type0.y = y_; type0.z = z_; }
	
	// Constructs a position on the terrain underneath the screen-space coordinates
	Position(const wxPoint& pt); // (implementation in ScenarioEditor.cpp)

	// Store all possible position representations in a union, instead of something
	// like inheritance and virtual functions, so we don't have to care about
	// dynamic memory allocation across DLLs.
	int type;
	union {
		struct { float x, y, z; } type0; // world-space coordinates
		struct { int x, y; } type1; // screen-space coordinates, to be projected onto terrain
		// type2: "same as previous" (e.g. for elevation-editing when the mouse hasn't moved)
	};

	// Constructs a position with the meaning "same as previous", which is handled
	// in unspecified (but usually obvious) ways by different message handlers.
	static Position Unchanged() { Position p; p.type = 2; return p; }

	// Only for use in the game, not the UI.
	// Implementations in Misc.cpp.
	void GetWorldSpace(CVector3D& vec) const;
	void GetWorldSpace(CVector3D& vec, float h) const;
	void GetWorldSpace(CVector3D& vec, const CVector3D& prev) const;
	void GetScreenSpace(float& x, float& y) const;
};

SHAREABLE_STRUCT(Position);

}

#endif // SHAREDTYPES_H__
