#ifndef GAMELOOP_H__
#define GAMELOOP_H__

extern void (*Atlas_GLSetCurrent)(void* context);

struct GameLoopState
{
	int argc;
	char** argv;
	bool running;
	bool rendering;
	bool worldloaded;
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
