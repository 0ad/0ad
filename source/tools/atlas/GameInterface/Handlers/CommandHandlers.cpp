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

#include "MessageHandler.h"

#include "../CommandProc.h"

namespace AtlasMessage {


MESSAGEHANDLER(DoCommand)
{
	Command* c = NULL;
	cmdHandlers::const_iterator it = GetCmdHandlers().find("c" + *msg->name);
	if (it != GetCmdHandlers().end())
	{
		c = (it->second)(msg->data);
	}
	else
	{
		debug_warn(L"Unrecognised command");
		return;
	}

	GetCommandProc().Submit(c);
}

MESSAGEHANDLER(UndoCommand)
{
	UNUSED2(msg);
	GetCommandProc().Undo();
}

MESSAGEHANDLER(RedoCommand)
{
	UNUSED2(msg);
	GetCommandProc().Redo();
}

MESSAGEHANDLER(MergeCommand)
{
	UNUSED2(msg);
	GetCommandProc().Merge();
}


}
