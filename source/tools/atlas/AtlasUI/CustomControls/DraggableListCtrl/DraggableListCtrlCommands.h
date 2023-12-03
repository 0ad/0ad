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

class DraggableListCtrl;

class DragCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(DragCommand);

public:
	DragCommand(DraggableListCtrl* ctrl, long src, long tgt);
	bool Do();
	bool Undo();

private:
	bool Merge(AtlasWindowCommand* previousCommand);

	DraggableListCtrl* m_Ctrl;
	long m_Src, m_Tgt;

	std::vector<AtObj> m_OldData;
};


class DeleteCommand : public AtlasWindowCommand
{
	DECLARE_CLASS(DeleteCommand);

public:
	DeleteCommand(DraggableListCtrl* ctrl, long itemID);
	bool Do();
	bool Undo();

private:
	DraggableListCtrl* m_Ctrl;
	long m_ItemID;
	std::vector<AtObj> m_OldData;
};
