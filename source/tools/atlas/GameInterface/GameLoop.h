/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_GAMELOOP
#define INCLUDED_GAMELOOP

#include "ps/GameSetup/CmdLineArgs.h"

extern void (*Atlas_GLSetCurrent)(void* context);

class AtlasView;

void RendererIncrementalLoad();

struct GameLoopState
{
	GameLoopState() : running(false) {};

	CmdLineArgs args;

	bool running; // whether the Atlas game loop is still running
	AtlasView* view; // current 'view' (controls updates, rendering, etc)

	const void* glCanvas; // the wxGlCanvas to draw on
	float realFrameLength; ///< Real-time duration of the last frame, in seconds. Smoothed to avoid large jumps (TODO).

	struct Input
	{
		float scrollSpeed[6]; // [fwd, bwd, left, right, cw-rotation, ccw-rotation]. 0.0f for disabled.
		float zoomDelta;
	} input;
};

extern GameLoopState* g_AtlasGameLoop;

#endif // INCLUDED_GAMELOOP
