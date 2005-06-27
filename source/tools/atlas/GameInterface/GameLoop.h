struct GameLoopState
{
	int argc;
	char** argv;
	bool running;
	bool rendering;
	void* currentDC;
};

extern GameLoopState* g_GameLoop;