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
		((float*, data)) // width*height array, allocated with new[] (handler will delete[])
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
	std::wstring name;
	unsigned char* imagedata; // RGB*size*size, allocated with malloc (querier should free)
};
QUERY(GetTerrainGroupPreviews,
	  ((std::wstring, groupname))
	  ((int, imagewidth))
	  ((int, imageheight))
	  ,
	  ((std::vector<sTerrainGroupPreview>, previews))
	  );


//////////////////////////////////////////////////////////////////////////

struct sEntitiesListItem
{
	std::wstring id;
	std::wstring name;
	// ...
};
QUERY(GetEntitiesList,
	  , // no inputs
	  ((std::vector<sEntitiesListItem>, entities))
	  );

MESSAGE(EntityPreview,
		((std::wstring, id)) // or empty string => disable
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);

COMMAND(CreateEntity, NOMERGE,
		((std::wstring, id))
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

struct ePaintTerrainPriority { enum { HIGH, LOW }; };
COMMAND(PaintTerrain, MERGE,
		((Position, pos))
		((std::wstring, texture))
		((int, priority))
		);

//////////////////////////////////////////////////////////////////////////

#include "MessagesSetup.h"

#endif // MESSAGES_H__
