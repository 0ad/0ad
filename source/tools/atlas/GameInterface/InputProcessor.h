#ifndef INCLUDED_INPUTPROCESSOR
#define INCLUDED_INPUTPROCESSOR

#include "GameLoop.h"

class InputProcessor
{
public:
	// Returns true if the camera has moved
	bool ProcessInput(GameLoopState* state);
};

#endif // INCLUDED_INPUTPROCESSOR
