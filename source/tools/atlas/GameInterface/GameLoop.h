#ifndef INCLUDED_GAMELOOP
#define INCLUDED_GAMELOOP

#include "ps/GameSetup/CmdLineArgs.h"

extern void (*Atlas_GLSetCurrent)(void* context);

class View;

struct GameLoopState
{
	CmdLineArgs args;

	bool running; // whether the Atlas game loop is still running
	View* view; // current 'view' (controls updates, rendering, etc)

	const void* glCanvas; // the wxGlCanvas to draw on
	float frameLength; // smoothed to avoid large jumps

	struct Input
	{
		float scrollSpeed[4]; // [fwd, bwd, left, right]. 0.0f for disabled.
		float zoomDelta;
	} input;
};

extern GameLoopState* g_GameLoop;

#endif // INCLUDED_GAMELOOP
