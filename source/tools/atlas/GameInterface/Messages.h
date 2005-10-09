#ifndef MESSAGES_H__
#define MESSAGES_H__

#include "MessagesSetup.h"

//////////////////////////////////////////////////////////////////////////

COMMAND(CommandString,
		((std::string, name))
		);

//////////////////////////////////////////////////////////////////////////

COMMAND(SetContext,
		((void*, context))
		);

COMMAND(ResizeScreen,
		((int, width))
		((int, height))
		);

//////////////////////////////////////////////////////////////////////////

COMMAND(GenerateMap,
		((int, size)) // size in number of patches
		);

COMMAND(LoadMap,
		((std::wstring, filename))
		);

//////////////////////////////////////////////////////////////////////////

COMMAND(RenderStyle,
		((bool, wireframe))
		);

COMMAND(MessageTrace,
		((bool, enable))
		);

//////////////////////////////////////////////////////////////////////////

COMMAND(Brush,
		((int, width)) // number of vertices
		((int, height))
		((float*, data)) // width*height array, allocated with new[]
		);

COMMAND(BrushPreview,
		((bool, enable))
		((Position, pos)) // only used if enable==true
		);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


struct eScrollConstantDir { enum { FORWARDS, BACKWARDS, LEFT, RIGHT }; };
INPUT(ScrollConstant,
	  ((int, dir))
	  ((float, speed)) // set speed 0.0f to stop scrolling
	  );

struct eScrollType { enum { FROM, TO }; };
INPUT(Scroll, // for scrolling by dragging the mouse FROM somewhere TO elsewhere
	  ((int, type))
	  ((Position, pos))
	  );

INPUT(SmoothZoom,
	  ((float, amount))
	  );

struct eRotateAroundType { enum { FROM, TO }; };
INPUT(RotateAround,
	  ((int, type))
	  ((Position, pos))
	  );

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WORLDCOMMAND(AlterElevation, MERGE,
			 ((Position, pos))
			 ((float, amount))
			 );

//////////////////////////////////////////////////////////////////////////

#include "MessagesSetup.h"

#endif // MESSAGES_H__
