/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_MESSAGES
#define INCLUDED_MESSAGES

#ifndef MESSAGES_SKIP_SETUP
#include "MessagesSetup.h"
#endif

#include <vector>
#include <string>

// TODO: organisation, documentation, etc

//////////////////////////////////////////////////////////////////////////

// Initialise some engine code. Must be called before anything else.
MESSAGE(Init, );

// Initialise graphics-related code. Must be called after the first SetCanvas,
// and before much else.
MESSAGE(InitGraphics, );

// Shut down engine/graphics code.
MESSAGE(Shutdown, );

struct eRenderView { enum renderViews { NONE, GAME, ACTOR }; };

MESSAGE(RenderEnable,
		((int, view)) // eRenderView
		);

// SetViewParam: used for hints to the renderer, e.g. to set wireframe mode;
// unrecognised param names are ignored
MESSAGE(SetViewParamB,
		((int, view)) // eRenderView
		((std::wstring, name))
		((bool, value))
		);
MESSAGE(SetViewParamI,
		((int, view)) // eRenderView
		((std::wstring, name))
		((int, value))
		);
MESSAGE(SetViewParamC,
		((int, view)) // eRenderView
		((std::wstring, name))
		((Colour, value))
		);
MESSAGE(SetViewParamS,
		((int, view)) // eRenderView
		((std::wstring, name))
		((std::wstring, value))
		);

MESSAGE(JavaScript,
		((std::wstring, command))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(GuiSwitchPage,
		((std::wstring, page))
		);

MESSAGE(GuiMouseButtonEvent,
		((int, button))
		((bool, pressed))
		((Position, pos))
		);

MESSAGE(GuiMouseMotionEvent,
		((Position, pos))
		);

MESSAGE(GuiKeyEvent,
		((int, sdlkey)) // SDLKey code
		((int, unichar)) // Unicode character
		((bool, pressed))
		);

MESSAGE(GuiCharEvent,
		((int, sdlkey))
		((int, unichar))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(SimStateSave,
		((std::wstring, label)) // named slot to store saved data
		);

MESSAGE(SimStateRestore,
		((std::wstring, label)) // named slot to find saved data
		);

QUERY(SimStateDebugDump,
		((bool, binary))
		,
		((std::wstring, dump))
		);

MESSAGE(SimPlay,
		((float, speed)) // 0 for pause, 1 for normal speed
		((bool, simTest)) // true if we're in simulation test mode, false otherwise
		);

//////////////////////////////////////////////////////////////////////////

QUERY(Ping, , );

//////////////////////////////////////////////////////////////////////////

MESSAGE(SetCanvas,
		((void*, canvas))
		((int, width))
		((int, height))
		);

MESSAGE(ResizeScreen,
		((int, width))
		((int, height))
		);

//////////////////////////////////////////////////////////////////////////
// Messages for map panel

QUERY(GenerateMap,
		((std::wstring, filename))				// random map script filename
		((std::string, settings))				// map settings as JSON string
		,
		((int, status))
		);

MESSAGE(ImportHeightmap,
		((std::wstring, filename))
		);

MESSAGE(LoadMap,
		((std::wstring, filename))
		);

MESSAGE(SaveMap,
		((std::wstring, filename))
		);

QUERY(GetMapList,
		,
		((std::vector<std::wstring>, scenarioFilenames))
		((std::vector<std::wstring>, skirmishFilenames))
		);

QUERY(GetMapSettings,
		,
		((std::string, settings))
		);

COMMAND(SetMapSettings, MERGE,
		((std::string, settings))
		);

MESSAGE(LoadPlayerSettings,
		((bool, newplayers))
		);

QUERY(GetMapSizes,
		,
		((std::string, sizes))
		);

QUERY(GetRMSData,
		,
		((std::vector<std::string>, data))
		);

COMMAND(ResizeMap, NOMERGE,
		((int, tiles))
		);

QUERY(VFSFileExists,
		((std::wstring, path))
		,
		((bool, exists))
		);

//////////////////////////////////////////////////////////////////////////
// Messages for player panel

QUERY(GetCivData,
		,
		((std::vector<std::string>, data))
		);

QUERY(GetPlayerDefaults,
		,
		((std::string, defaults))
		);

QUERY(GetAIData,
		,
		((std::string, data))
		);

//////////////////////////////////////////////////////////////////////////

MESSAGE(RenderStyle,
		((bool, wireframe))
		);

MESSAGE(MessageTrace,
		((bool, enable))
		);

MESSAGE(Screenshot,
		((bool, big))
		((int, tiles)) // For big screenshots: the final image will be (640*tiles)x(480*tiles)
		);

#ifndef MESSAGES_SKIP_STRUCTS
struct sCinemaRecordCB
{
	unsigned char* buffer;
};
SHAREABLE_STRUCT(sCinemaRecordCB);
#endif

QUERY(CinemaRecord,
	  ((std::wstring, path))
	  ((int, framerate))
	  ((float, duration))
	  ((int, width))
	  ((int, height))
	  ((Callback<sCinemaRecordCB>, cb))
	  ,
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
	  ((std::vector<std::wstring>, groupNames))
	  );

#ifndef MESSAGES_SKIP_STRUCTS
struct sTerrainTexturePreview
{
	Shareable<std::wstring> name;
	Shareable<bool> loaded;
	Shareable<int> imageWidth;
	Shareable<int> imageHeight;
	Shareable<std::vector<unsigned char> > imageData; // RGB*width*height
};
SHAREABLE_STRUCT(sTerrainTexturePreview);
#endif

QUERY(GetTerrainGroupPreviews,
	  ((std::wstring, groupName))
	  ((int, imageWidth))
	  ((int, imageHeight))
	  ,
	  ((std::vector<sTerrainTexturePreview>, previews))
	  );

QUERY(GetTerrainPassabilityClasses,
	  , // no inputs
	  ((std::vector<std::wstring>, classNames))
	  );

QUERY(GetTerrainTexturePreview,
		((std::wstring, name))
		((int, imageWidth))
		((int, imageHeight))
		,
		((sTerrainTexturePreview, preview))
		);

//////////////////////////////////////////////////////////////////////////

#ifndef MESSAGES_SKIP_STRUCTS
struct sObjectsListItem
{
	Shareable<std::wstring> id;
	Shareable<std::wstring> name;
	Shareable<int> type; // 0 = entity, 1 = actor
};
SHAREABLE_STRUCT(sObjectsListItem);
#endif

QUERY(GetObjectsList,
	  , // no inputs
	  ((std::vector<sObjectsListItem>, objects)) // sorted by .name
	  );

#ifndef MESSAGES_SKIP_STRUCTS
struct sObjectSettings
{
	Shareable<int> player;
	Shareable<std::vector<std::wstring> > selections;

	// Some settings are immutable and therefore are ignored (and should be left
	// empty) when passed from the editor to the game:

	Shareable<std::vector<std::vector<std::wstring> > > variantGroups;
};
SHAREABLE_STRUCT(sObjectSettings);
#endif

// transform de local entity to a real entity
MESSAGE(ObjectPreviewToEntity,);

//Query for get selected objects
QUERY(GetCurrentSelection,
	, //No inputs
	((std::vector<ObjectID>, ids))
	);

// Moving Preview(s) object together, default is using the firs element in vector
MESSAGE(MoveObjectPreview,
		((Position,pos))
		);

// Preview object in the game world - creates a temporary unit at the given
// position, and removes it when the preview is next changed
MESSAGE(ObjectPreview,
		((std::wstring, id)) // or empty string => disable
		((sObjectSettings, settings))
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		((unsigned int, actorseed))
		((bool, cleanObjectPreviews))
		);

COMMAND(CreateObject, NOMERGE,
		((std::wstring, id))
		((sObjectSettings, settings))
		((Position, pos))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		((unsigned int, actorseed))
		);

// Set an actor to be previewed on its own (i.e. without the game world).
// (Use RenderEnable to make it visible.)
MESSAGE(SetActorViewer,
		((std::wstring, id))
		((std::wstring, animation))
		((int, playerID))
		((float, speed))
		((bool, flushcache)) // true => unload all actor files before starting the preview (because we don't have proper hotloading yet)
		);

//////////////////////////////////////////////////////////////////////////

QUERY(Exit,,); // no inputs nor outputs

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct eScrollConstantDir { enum { FORWARDS, BACKWARDS, LEFT, RIGHT, CLOCKWISE, ANTICLOCKWISE }; };
MESSAGE(ScrollConstant, // set a constant scrolling(/rotation) rate
		((int, view)) // eRenderView
		((int, dir)) // eScrollConstantDir
		((float, speed)) // set speed 0.0f to stop scrolling
		);

struct eScrollType { enum { FROM, TO }; };
MESSAGE(Scroll, // for scrolling by dragging the mouse FROM somewhere TO elsewhere
		((int, view)) // eRenderView
		((int, type)) // eScrollType
		((Position, pos))
		);

MESSAGE(SmoothZoom,
		((int, view)) // eRenderView
		((float, amount))
		);

struct eRotateAroundType { enum { FROM, TO }; };
MESSAGE(RotateAround,
		((int, view)) // eRenderView
		((int, type)) // eRotateAroundType
		((Position, pos))
		);

MESSAGE(LookAt,
		((int, view)) // eRenderView
		((Position, pos))
		((Position, target))
		);

MESSAGE(CameraReset, );

QUERY(GetView,
		,
		((sCameraInfo, info))
		);

MESSAGE(SetView,
		((sCameraInfo, info))
		);

//////////////////////////////////////////////////////////////////////////

#ifndef MESSAGES_SKIP_STRUCTS
struct sEnvironmentSettings
{
	Shareable<float> waterheight; // range 0..1 corresponds to min..max terrain height; out-of-bounds values allowed
	Shareable<float> waterwaviness; // range ???
	Shareable<float> watermurkiness; // range ???
	
	Shareable<Colour> watercolour;
	Shareable<Colour> watertint;
	Shareable<Colour> waterreflectiontint;
	Shareable<float> waterreflectiontintstrength; // range ???

	Shareable<float> sunrotation; // range -pi..+pi
	Shareable<float> sunelevation; // range -pi/2 .. +pi/2

	// emulate 'HDR' by allowing overly bright suncolour. this is
	// multiplied on to suncolour after converting to float
	// (struct Colour stores as normal u8, 0..255)
	Shareable<float> sunoverbrightness; // range 1..3

	// support different lighting models ("old" for the version compatible with old scenarios,
	// "standard" for the new normal model that supports much brighter lighting)
	Shareable<std::wstring> posteffect;

	Shareable<std::wstring> skyset;

	Shareable<Colour> suncolour;
	Shareable<Colour> terraincolour;
	Shareable<Colour> unitcolour;
	Shareable<Colour> fogcolour;
	
	Shareable<float> fogfactor;
	Shareable<float> fogmax;
	
	Shareable<float> brightness;
	Shareable<float> contrast;
	Shareable<float> saturation;
	Shareable<float> bloom;
};
SHAREABLE_STRUCT(sEnvironmentSettings);
#endif

QUERY(GetEnvironmentSettings,
	  // no inputs
	  ,
	  ((sEnvironmentSettings, settings))
	  );

COMMAND(SetEnvironmentSettings, MERGE,	// merge lots of small changes into one undoable command
		((sEnvironmentSettings, settings))
		);

COMMAND(RecalculateWaterData, NOMERGE, ((float,unused)));

QUERY(GetSkySets,
	  // no inputs
	  ,
	  ((std::vector<std::wstring>, skysets))
	  );

QUERY(GetPostEffects,
	  // no inputs
	  ,
	  ((std::vector<std::wstring>, posteffects))
	  );


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

COMMAND(AlterElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

COMMAND(SmoothElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

COMMAND(FlattenElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

COMMAND(PikeElevation, MERGE,
		((Position, pos))
		((float, amount))
		);

struct ePaintTerrainPriority { enum { HIGH, LOW }; };
COMMAND(PaintTerrain, MERGE,
		((Position, pos))
		((std::wstring, texture))
		((int, priority)) // ePaintTerrainPriority
		);

COMMAND(ReplaceTerrain, NOMERGE,
		((Position, pos))
		((std::wstring, texture))
		);

COMMAND(FillTerrain, NOMERGE,
		((Position, pos))
		((std::wstring, texture))
		);

QUERY(GetTerrainTexture,
		((Position, pos))
		,
		((std::wstring, texture))
		);

//////////////////////////////////////////////////////////////////////////

QUERY(PickObject,
		((Position, pos))
		((bool, selectActors))
		,
		((ObjectID, id))
		((int, offsetx))  // offset of object centre from input position
		((int, offsety)) //
		);

QUERY(PickObjectsInRect,
		((Position, start))
		((Position, end))
		((bool, selectActors))
		,
		((std::vector<ObjectID>, ids))
		);

QUERY(PickSimilarObjects,
		((ObjectID, id))
		,
		((std::vector<ObjectID>, ids))
		);

COMMAND(MoveObjects, MERGE,
		((std::vector<ObjectID>, ids))
		((ObjectID, pivot))
		((Position, pos))
		);

COMMAND(RotateObject, MERGE,
		((ObjectID, id))
		((bool, usetarget)) // true => use 'target' for orientation; false => use 'angle'
		((Position, target))
		((float, angle))
		);

COMMAND(DeleteObjects, NOMERGE,
		((std::vector<ObjectID>, ids))
		);

MESSAGE(SetSelectionPreview,
		((std::vector<ObjectID>, ids))
		);

QUERY(GetObjectSettings,
		((int, view)) // eRenderView
		((ObjectID, id))
		,
		((sObjectSettings, settings))
		);

COMMAND(SetObjectSettings, NOMERGE,
		((int, view)) // eRenderView
		((ObjectID, id))
		((sObjectSettings, settings))
		);

QUERY(GetObjectMapSettings,
		((std::vector<ObjectID>, ids))
		,
		((std::wstring, xmldata))
		);


QUERY(GetPlayerObjects,
		((int, player))
		,
		((std::vector<ObjectID>, ids))
		);

MESSAGE(SetBandbox,
		((bool, show))
		((int, sx0))
		((int, sy0))
		((int, sx1))
		((int, sy1))
		);

//////////////////////////////////////////////////////////////////////////

QUERY(GetCinemaPaths,
	  , // no inputs
	  ((std::vector<AtlasMessage::sCinemaPath> , paths))
	  );

QUERY(GetCameraInfo,
	  ,
	  ((AtlasMessage::sCameraInfo, info))
	  );

COMMAND(SetCinemaPaths, NOMERGE,
		((std::vector<AtlasMessage::sCinemaPath>, paths))
		);

MESSAGE(CinemaEvent,
		((std::wstring, path))
		((int, mode))
		((float, t))
		((bool, drawCurrent))
		((bool, lines))
		);

//////////////////////////////////////////////////////////////////////////

enum eTriggerListType
{
	CINEMA_LIST,
	TRIGGER_LIST,
	TRIG_GROUP_LIST	//list of trigger groups
	// [Eventually include things like entities and areas as the editor progresses...]
};


QUERY(GetTriggerData,
	  , //no inputs
	  ((std::vector<AtlasMessage::sTriggerGroup>, groups))
	  ((std::vector<AtlasMessage::sTriggerSpec>, conditions))
	  ((std::vector<AtlasMessage::sTriggerSpec>, effects))
	  );

QUERY(GetTriggerChoices,
	  ((std::wstring, name)),
	  ((std::vector<std::wstring>, choices))
	  ((std::vector<std::wstring>, translations))
	  );
		
COMMAND(SetAllTriggers, NOMERGE, 
	  ((std::vector<AtlasMessage::sTriggerGroup>, groups))
	  );

QUERY(GetWorldPosition,
	  ((int, x))
	  ((int, y)),
	  ((Position, position))
	  );

MESSAGE(TriggerToggleSelector,
		((bool, enable))
		((Position, position))
		);


#ifndef MESSAGES_SKIP_SETUP
#include "MessagesSetup.h"
#endif

#endif // INCLUDED_MESSAGES
