/* Copyright (C) 2017 Wildfire Games.
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

#ifndef DLLINTERFACE_INCLUDED
#define DLLINTERFACE_INCLUDED

#include <wchar.h>

namespace AtlasMessage { class MessagePasser; }

ATLASDLLIMPEXP void Atlas_SetMessagePasser(AtlasMessage::MessagePasser*);
ATLASDLLIMPEXP void Atlas_SetDataDirectory(const wchar_t* path);
ATLASDLLIMPEXP void Atlas_StartWindow(const wchar_t* type);

ATLASDLLIMPEXP void Atlas_GLSetCurrent(void* context);
ATLASDLLIMPEXP void Atlas_GLSwapBuffers(void* context);

ATLASDLLIMPEXP void Atlas_NotifyEndOfFrame();

ATLASDLLIMPEXP void Atlas_DisplayError(const wchar_t* text, size_t flags);

#endif // DLLINTERFACE_INCLUDED
