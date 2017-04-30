/* Copyright (C) 2017 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SHAREDTYPES
#define INCLUDED_SHAREDTYPES

#include "Shareable.h"

#include <string>

class wxPoint;
class CVector3D;

namespace AtlasMessage
{

enum SELECTED_AXIS
{
	AXIS_INVALID = -1,
	AXIS_X = 1,
	AXIS_Y = 2,
	AXIS_Z = 4
};

// Represents a position in the game world, with an interface usable from the
// UI (which usually knows only about screen space). Typically constructed
// by the UI, then passed to the game which uses GetWorldSpace.

struct Position
{
	Position() : type(0) { type0.x = type0.y = type0.z = 0.f;  }

	// Constructs a position with specified world-space coordinates
	Position(float x, float y, float z) : type(0) { type0.x = x; type0.y = y; type0.z = z; }

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
	CVector3D GetWorldSpace(bool floating = false) const;
	CVector3D GetWorldSpace(float h, bool floating = false) const;
	CVector3D GetWorldSpace(const CVector3D& prev, bool floating = false) const;
	void GetScreenSpace(float& x, float& y) const;
};
SHAREABLE_STRUCT(Position);


struct Color
{
	Color()
		: r(255), g(0), b(255)
	{
	}

	Color(unsigned char r, unsigned char g, unsigned char b)
		: r(r), g(g), b(b)
	{
	}

	unsigned char r, g, b;
};
SHAREABLE_STRUCT(Color);


typedef unsigned int ObjectID;
inline bool ObjectIDIsValid(ObjectID id) { return (id != 0); }


struct sCinemaSplineNode
{
	Shareable<float> px, py, pz, rx, ry, rz, t;
public:
	sCinemaSplineNode(float _px, float _py, float _pz, float _rx, float _ry, float _rz)
			: px(_px), py(_py), pz(_pz), rx(_rx), ry(_ry), rz(_rz), t(0.0f) {}
	sCinemaSplineNode() {}
	void SetTime(float _t) { t = _t; }
};
SHAREABLE_STRUCT(sCinemaSplineNode);

struct sCinemaPath
{
	Shareable<std::vector<AtlasMessage::sCinemaSplineNode> > nodes;
	Shareable<std::wstring> name;
	Shareable<float> duration, timescale;
	Shareable<int> mode, growth, change, style;	// change == switch point

	sCinemaPath(const std::wstring& _name) : name(_name), mode(0), style(0), change(0), growth(0), duration(0), timescale(1) {}
	sCinemaPath() : mode(0), style(0), change(0), growth(0), duration(0), timescale(1) {}

	/*AtlasMessage::sCinemaPath operator-(const AtlasMessage::sCinemaPath& path)
	{
		return AtlasMessage::sCinemaPath(x - path.x, y - path.y, z - path.z);
	}
	AtlasMessage::sCinemaPath operator+(const AtlasMessage::sCinemaPath& path)
	{
		return AtlasMessage::sCinemaPath(x + path.x, y + path.y, z + path.z);
	}*/
};
SHAREABLE_STRUCT(sCinemaPath);

struct sCinemaPathNode
{
	Shareable<std::wstring> name;
	Shareable<int> index;
	Shareable<bool> targetNode;
	sCinemaPathNode() : index(-1), targetNode(false) {}
};
SHAREABLE_STRUCT(sCinemaPathNode);


struct eCinemaEventMode { enum { SMOOTH, SELECT, IMMEDIATE_PATH, RESET }; };
struct sCameraInfo
{
	Shareable<float> pX, pY, pZ, rX, rY, rZ;	// position and rotation
};
SHAREABLE_STRUCT(sCameraInfo);


//******Triggers*****


struct sTriggerParameter
{
	Shareable<int> row, column, xPos, yPos, xSize, ySize, parameterOrder;
	Shareable<std::wstring> name, inputType, windowType;
};
SHAREABLE_STRUCT(sTriggerParameter);

struct sTriggerSpec
{
	Shareable<std::vector<AtlasMessage::sTriggerParameter> > parameters;
	Shareable<std::wstring> displayName, functionName;

	bool operator== ( const std::wstring& name) const
	{
		return ( *displayName == name );
	}
};
SHAREABLE_STRUCT(sTriggerSpec);


struct sTriggerCondition
{
	sTriggerCondition() : linkLogic(1), negated(false) {}
	sTriggerCondition(const std::wstring& _name) : linkLogic(1), negated(false), name(_name) {}

	//displayName is used for selecting choice items in Atlas
	Shareable<std::wstring> name, functionName, displayName;
	Shareable< std::vector<std::wstring> > parameters;

	//0 = none, 1 = and, 2 = or
	Shareable<int> linkLogic;
	Shareable<bool> negated;

	bool operator== ( const std::wstring& _name ) const
	{
		return (*name == _name);
	}
};

SHAREABLE_STRUCT(sTriggerCondition);

struct sTriggerEffect
{
	sTriggerEffect() {}
	sTriggerEffect(const std::wstring& _name) : name(_name) {}

	Shareable<std::wstring> name, functionName, displayName;
	Shareable< std::vector<std::wstring> > parameters;
	//Shareable<bool> loop;

	bool operator== ( const std::wstring& _name )
	{
		return (*name == _name);
	}
};
SHAREABLE_STRUCT(sTriggerEffect);

struct sTrigger
{
	Shareable<std::wstring> name, group;
	Shareable< std::vector<sTriggerCondition> > conditions;
	Shareable< std::vector<sTriggerEffect> > effects;

	//For beginnings, the index of the term it comes before.  For ends, the index it comes after
	Shareable< std::vector<int> > logicBlocks, logicBlockEnds;
	Shareable< std::vector<bool> > logicNots;	//true if logicBlocks are not-ed
	Shareable<float> timeValue;
	Shareable<int> maxRuns;
	Shareable<bool> active;

	sTrigger() : timeValue(0), maxRuns(-1), active(0) {}
	sTrigger(const std::wstring& _name) : name(_name), timeValue(0), maxRuns(0), active(true) {}
	bool operator== (const sTrigger& trigger)
	{
		return (*name == *trigger.name);
	}
	bool operator!= (const sTrigger& trigger)
	{
		return (*name != *trigger.name);
	}
};
SHAREABLE_STRUCT(sTrigger);


struct sTriggerGroup
{
	Shareable<std::wstring> name, parentName;
	Shareable< std::vector<std::wstring> > children;
	Shareable< std::vector<sTrigger> > triggers;

	sTriggerGroup() { }
	sTriggerGroup(const std::wstring& _name) : name(_name) { }

	bool operator== (const sTriggerGroup& group) const
	{
		return (*name == *group.name);
	}
	bool operator== (const std::wstring& _name) const
	{
		return (*name == _name);
	}
	/*bool operator!= (const sTriggerGroup& group)
	{
		return (*name != *group.name);
	}*/
};

SHAREABLE_STRUCT(sTriggerGroup);


}

#endif // INCLUDED_SHAREDTYPES
