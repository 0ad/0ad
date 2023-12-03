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

#include "AtlasWindowCommand.h"

#include "IAtlasSerialiser.h"

IMPLEMENT_ABSTRACT_CLASS(AtlasWindowCommand, wxCommand);

IMPLEMENT_CLASS(AtlasCommand_Begin, AtlasWindowCommand);
IMPLEMENT_CLASS(AtlasCommand_End, AtlasWindowCommand);

AtlasCommand_Begin::AtlasCommand_Begin(const wxString& description, IAtlasSerialiser* object)
	: AtlasWindowCommand(true, description),
	m_Object(object),
	m_PreData(object->FreezeData())
{
}

bool AtlasCommand_Begin::Do()
{
	if (m_PostData.defined())
		m_Object->ThawData(m_PostData);
	return true;
}

bool AtlasCommand_Begin::Undo()
{
	m_Object->ThawData(m_PreData);
	return true;
}



bool AtlasCommand_End::Merge(AtlasWindowCommand* command)
{
	AtlasCommand_Begin* previousCommand = wxDynamicCast(command, AtlasCommand_Begin);

	if (! previousCommand)
	{
		wxLogError(_("Internal error - invalid _end merge"));
		return false;
	}

	previousCommand->m_PostData = previousCommand->m_Object->FreezeData();
	return true;
}
