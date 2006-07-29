#ifndef MESSAGES_H__
#define MESSAGES_H__

#ifndef MESSAGES_SKIP_SETUP
#include "MessagesSetup.h"
#endif

// TODO: organisation, documentation, etc

//////////////////////////////////////////////////////////////////////////

MESSAGE(Init, );

MESSAGE(Shutdown, );

MESSAGE(RenderEnable,
		((bool, enabled)));

//////////////////////////////////////////////////////////////////////////

QUERY(Ping, , );

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

struct sObjectSettings
{
	Shareable<int> player;
	Shareable<std::vector<std::string> > selections;

	// Some settings are immutable and therefore are ignored (and should be left
	// empty) when passed from the editor to the game:

	Shareable<std::vector<std::vector<std::string> > > variantgroups;
};
SHAREABLE_STRUCT(sObjectSettings);

MESSAGE(ObjectPreview,
		((std::wstring, id)) // or empty string => disable
		((sObjectSettings, settings))
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);

COMMAND(CreateObject, NOMERGE,
		((std::wstring, id))
		((sObjectSettings, settings))
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

struct sEnvironmentSettings
{
	Shareable<float> waterheight; // range 0..1 corresponds to min..max terrain height; out-of-bounds values allowed
	Shareable<float> watershininess;
	Shareable<float> waterwaviness;

	Shareable<float> sunrotation; // range -pi..+pi
	Shareable<float> sunelevation; // range -pi/2 .. +pi/2

	Shareable<std::wstring> skyset;

	Shareable<Colour> suncolour;
	Shareable<Colour> terraincolour;
	Shareable<Colour> unitcolour;
};
SHAREABLE_STRUCT(sEnvironmentSettings);

QUERY(GetEnvironmentSettings,
	  // no inputs
	  ,
	  ((sEnvironmentSettings, settings))
	  );

COMMAND(SetEnvironmentSettings, MERGE,
		((sEnvironmentSettings, settings))
		);

QUERY(GetSkySets,
	  // no inputs
	  ,
	  ((std::vector<std::wstring>, skysets))
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
FUNCTION(
inline bool ObjectIDIsValid(ObjectID id) { return (id >= 0); }
);

QUERY(PickObject,
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

QUERY(GetObjectSettings,
	  ((ObjectID, id))
	  ,
	  ((sObjectSettings, settings))
	  );

COMMAND(SetObjectSettings, NOMERGE,
		((ObjectID, id))
		((sObjectSettings, settings))
		);

//////////////////////////////////////////////////////////////////////////

struct sCinemaSplineNode
{
	Shareable<float> x, y, z, t;
};
SHAREABLE_STRUCT(sCinemaSplineNode);

struct sCinemaPath
{
	Shareable<std::vector<AtlasMessage::sCinemaSplineNode> > nodes;
	Shareable<float> duration, x, y, z;
	Shareable<int> mode, growth, change, style;	//change == switch point
};
SHAREABLE_STRUCT(sCinemaPath);

struct sCinemaTrack
{
	Shareable<std::wstring> name;
	Shareable<float> x, y, z, timescale, duration;
	Shareable<std::vector<AtlasMessage::sCinemaPath> > paths;
};
SHAREABLE_STRUCT(sCinemaTrack);

struct eCinemaMovementMode { enum { SMOOTH, IMMEDIATE_PATH, IMMEDIATE_TRACK }; };

struct sCinemaIcon
{
	Shareable<std::wstring> name;
	Shareable<std::vector<unsigned char> > imageData;
};
SHAREABLE_STRUCT(sCinemaIcon);

QUERY(GetCinemaTracks,
	  , //no input
	  ((std::vector<AtlasMessage::sCinemaTrack> , tracks))
	  );

QUERY(GetCinemaIcons,
	  ,
	  ((std::vector<AtlasMessage::sCinemaIcon>, images))
	  );


COMMAND(SetCinemaTracks, MERGE,
		((std::vector<AtlasMessage::sCinemaTrack>, tracks))
		((float, timescale))
		);

MESSAGE(CinemaMovement,
		((std::wstring, track))
		((int, mode))
		((float, t))
		);

//////////////////////////////////////////////////////////////////////////

#ifndef MESSAGES_SKIP_SETUP
#include "MessagesSetup.h"
#endif

#endif // MESSAGES_H__
