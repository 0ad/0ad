#ifndef MESSAGES_H__
#define MESSAGES_H__

#include "MessagesSetup.h"

// TODO: organisation, documentation, etc

//////////////////////////////////////////////////////////////////////////

MESSAGE(CommandString,
		((std::string, name))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(SetContext,
		((void*, context))
		);

MESSAGE(ResizeScreen,
		((int, width))
		((int, height))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(GenerateMap,
		((int, size)) // size in number of patches
		);

MESSAGE(LoadMap,
		((std::wstring, filename))
		);

MESSAGE(SaveMap,
		((std::wstring, filename))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(RenderStyle,
		((bool, wireframe))
		);

MESSAGE(MessageTrace,
		((bool, enable))
		);

MESSAGE(Screenshot,
		((int, tiles)) // the final image will be (640*tiles)x(480*tiles)
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(Brush,
		((int, width)) // number of vertices
		((int, height))
		((std::vector<float>, data)) // width*height array
		);

MESSAGE(BrushPreview,
		((bool, enable))
		((Position, pos)) // only used if enable==true
		);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

QUERY(GetTerrainGroups,
	  , // no inputs
	  ((std::vector<std::wstring>, groupnames))
	  );

struct sTerrainGroupPreview
{
	Shareable<std::wstring> name;
	Shareable<std::vector<unsigned char> > imagedata; // RGB*size*size
};
SHAREABLE_STRUCT(sTerrainGroupPreview);

QUERY(GetTerrainGroupPreviews,
	  ((std::wstring, groupname))
	  ((int, imagewidth))
	  ((int, imageheight))
	  ,
	  ((std::vector<sTerrainGroupPreview>, previews))
	  );


//////////////////////////////////////////////////////////////////////////

struct sObjectsListItem
{
	Shareable<std::wstring> id;
	Shareable<std::wstring> name;
	Shareable<int> type; // 0 = entity, 1 = actor
};
SHAREABLE_STRUCT(sObjectsListItem);

QUERY(GetObjectsList,
	  , // no inputs
	  ((std::vector<sObjectsListItem>, objects))
	  );

struct sUnitSettings
{
	Shareable<int> player;
	Shareable<std::vector<std::wstring> > selections;
};
SHAREABLE_STRUCT(sUnitSettings);

MESSAGE(ObjectPreview,
		((std::wstring, id)) // or empty string => disable
		((sUnitSettings, settings))
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);

COMMAND(CreateObject, NOMERGE,
		((std::wstring, id))
		((sUnitSettings, settings))
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);


//////////////////////////////////////////////////////////////////////////

QUERY(Exit,,); // no inputs nor outputs

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


struct eScrollConstantDir { enum { FORWARDS, BACKWARDS, LEFT, RIGHT }; };
MESSAGE(ScrollConstant,
		((int, dir))
		((float, speed)) // set speed 0.0f to stop scrolling
		);

struct eScrollType { enum { FROM, TO }; };
MESSAGE(Scroll, // for scrolling by dragging the mouse FROM somewhere TO elsewhere
		((int, type))
		((Position, pos))
		);

MESSAGE(SmoothZoom,
		((float, amount))
		);

struct eRotateAroundType { enum { FROM, TO }; };
MESSAGE(RotateAround,
		((int, type))
		((Position, pos))
		);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

COMMAND(AlterElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

COMMAND(FlattenElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

struct ePaintTerrainPriority { enum { HIGH, LOW }; };
COMMAND(PaintTerrain, MERGE,
		((Position, pos))
		((std::wstring, texture))
		((int, priority))
		);

//////////////////////////////////////////////////////////////////////////

typedef int ObjectID;
inline bool ObjectIDIsValid(ObjectID id) { return (id >= 0); }

QUERY(SelectObject,
	  ((Position, pos))
	  ,
	  ((ObjectID, id))
	  ((int, offsetx))  // offset of object centre from input position
	  ((int, offsety)) //
	  );

COMMAND(MoveObject, MERGE,
		((ObjectID, id))
		((Position, pos))
		);

COMMAND(RotateObject, MERGE,
		((ObjectID, id))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);

COMMAND(DeleteObject, NOMERGE,
		((ObjectID, id))
		);

MESSAGE(SetSelectionPreview,
		((std::vector<ObjectID>, ids))
		);

//////////////////////////////////////////////////////////////////////////

#include "MessagesSetup.h"

#endif // MESSAGES_H__
