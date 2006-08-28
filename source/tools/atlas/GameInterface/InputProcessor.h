#ifndef INPUTPROCESSOR_H__
#define INPUTPROCESSOR_H__

#include "GameLoop.h"

class InputProcessor
{
public:
	// Returns true if the camera has moved
	bool ProcessInput(GameLoopState* state);
};

#endif // INPUTPROCESSOR_H__
