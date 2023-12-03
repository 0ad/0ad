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

#include "General/AtlasWindowCommand.h"

#include "AtlasObject/AtlasObject.h"

#include <vector>

class EditableListCtrl;

class EditCommand_Dialog : public AtlasWindowCommand
{
	DECLARE_CLASS(EditCommand_Dialog);

public:
	EditCommand_Dialog(EditableListCtrl* ctrl, long row, int col, AtObj& newData);

	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	int m_Col;
	AtObj m_NewData;
	std::vector<AtObj> m_OldData;

};


class EditCommand_Text : public AtlasWindowCommand
{
	DECLARE_CLASS(EditCommand_Text);

public:
	EditCommand_Text(EditableListCtrl* ctrl, long row, int col, wxString newText);

	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	int m_Col;
	wxString m_NewText;
	std::vector<AtObj> m_OldData;
};


class PasteCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(PasteCommand);

public:
	PasteCommand(EditableListCtrl* ctrl, long row, AtObj& newData);
	bool Do();
	bool Undo();

private:
	EditableListCtrl* m_Ctrl;
	long m_Row;
	AtObj m_NewData;
	std::vector<AtObj> m_OldData;
};

