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

#ifndef INCLUDED_ATLASWINDOWCOMMAND
#define INCLUDED_ATLASWINDOWCOMMAND

#include "wx/cmdproc.h"
#include "AtlasObject/AtlasObject.h"
class IAtlasSerialiser;

class AtlasWindowCommand : public wxCommand
{
	DECLARE_ABSTRACT_CLASS(AtlasWindowCommand);

	friend class AtlasWindowCommandProc;

public:
	AtlasWindowCommand(bool canUndoIt, const wxString& name)
		: wxCommand(canUndoIt, name), m_Finalized(false)
	{ }

	// Control merging of this command with a future one (so they
	// can be undone in a single step). Called after 'Do'.
	// The function should try to merge with the previous command,
	// by altering that previous command and then returning true.
	// If it can't, return false.
	virtual bool Merge(AtlasWindowCommand* WXUNUSED(previousCommand)) { return false; }

private:
	bool m_Finalized;
};

//////////////////////////////////////////////////////////////////////////

class AtlasCommand_Begin : public AtlasWindowCommand
{
	DECLARE_CLASS(AtlasCommand_Begin);

	friend class AtlasCommand_End;

public:
	AtlasCommand_Begin(const wxString& description, IAtlasSerialiser* object);

	bool Do();
	bool Undo();

private:
	IAtlasSerialiser* m_Object;
	AtObj m_PreData, m_PostData;
};

class AtlasCommand_End : public AtlasWindowCommand
{
	DECLARE_CLASS(AtlasCommand_End);

public:
	AtlasCommand_End() : AtlasWindowCommand(true, _T("[error]")) {}

	bool Do() { return true; }
	bool Undo() { return false; }
	bool Merge(AtlasWindowCommand* previousCommand);
};


#endif // INCLUDED_ATLASWINDOWCOMMAND
