/* Copyright (C) 2009 Wildfire Games.
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

#include "precompiled.h"

#include "Messages.h"
#include "Handlers/MessageHandler.h"
#include "CommandProc.h"

// We want to include Messages.h again below, with some different definitions,
// so cheat and undefine its include-guard
#undef INCLUDED_MESSAGES

#include <map>
#include <string>

#include "SharedTypes.h"
#include "Shareable.h"

#include <map>
#include <string>

namespace AtlasMessage
{

#define MESSAGE(name, vals) \
	extern void f##name##_wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##_wrapper));

#define QUERY(name, in_vals, out_vals) \
	extern void f##name##_wrapper(AtlasMessage::IMessage*); \
	AtlasMessage::GetMsgHandlers().insert(std::pair<std::string, AtlasMessage::msgHandler>(#name, &f##name##_wrapper));

#define COMMAND(name, merge, vals) \
	extern cmdHandler c##name##_create(); \
	GetCmdHandlers().insert(std::pair<std::string, cmdHandler>("c"#name, c##name##_create()));

#undef SHAREABLE_STRUCT
#define SHAREABLE_STRUCT(name)

void RegisterHandlers()
{
	MESSAGE(DoCommand, );
	MESSAGE(UndoCommand, );
	MESSAGE(RedoCommand, );
	MESSAGE(MergeCommand, );

	#define MESSAGES_SKIP_SETUP
	#include "Messages.h"
}

}
