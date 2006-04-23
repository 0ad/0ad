#ifndef SHAREDTYPES_H__
#define SHAREDTYPES_H__

#include "Shareable.h"

class wxPoint;
class CVector3D;

namespace AtlasMessage
{

struct Position
{
	Position() : type(0) { type0.x = type0.y = type0.z = 0.f;  }
	Position(float x_, float y_, float z_) : type(0) { type0.x = x_; type0.y = y_; type0.z = z_; }
	Position(const wxPoint& pt); // (implementation in ScenarioEditor.cpp)

	int type;
	union {
		struct { float x, y, z; } type0; // world-space coordinates
		struct { int x, y; } type1; // screen-space coordinates, to be projected onto terrain
		// type2: "same as previous" (e.g. for elevation-editing when the mouse hasn't moved)
	};

	// Constructs a position with the meaning "same as previous", which is handled
	// in an unspecified way by various message handlers.
	static Position Unchanged() { Position p; p.type = 2; return p; }

	// Only for use in the game, not the UI.
	// Implementations in Misc.cpp.
	void GetWorldSpace(CVector3D& vec) const;
	void GetWorldSpace(CVector3D& vec, float h) const;
	void GetWorldSpace(CVector3D& vec, const CVector3D& prev) const;
	void GetScreenSpace(float& x, float& y) const;
};
SHAREABLE_POD(Position);

}

#endif // SHAREDTYPES_H__
