// ProfileViewer.h
//
// A temporary interface for viewing profile information

#ifndef PROFILE_VIEWER_INCLUDED
#define PROFILE_VIEWER_INCLUDED

#include "input.h"

void ResetProfileViewer();
void RenderProfile();
InReaction profilehandler( const SDL_Event* ev );

#endif
