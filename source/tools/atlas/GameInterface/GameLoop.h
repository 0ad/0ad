#ifndef GAMELOOP_H__
#define GAMELOOP_H__

extern void (*Atlas_GLSetCurrent)(void* context);

class View;

struct GameLoopState
{
	int argc;
	char** argv;

	bool running; // whether the Atlas game loop is still running
	View* view; // current 'view' (controls updates, rendering, etc)

	const void* glContext;
	float frameLength; // smoothed to avoid large jumps

	struct Input
	{
		float scrollSpeed[4]; // [fwd, bwd, left, right]. 0.0f for disabled.
		float zoomDelta;
	} input;
};

extern GameLoopState* g_GameLoop;

#endif // GAMELOOP_H__
