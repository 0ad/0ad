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

#include "DraggableListCtrlCommands.h"

#include "DraggableListCtrl.h"
#include "EditableListCtrl/EditableListCtrl.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(DragCommand, AtlasWindowCommand);

DragCommand::DragCommand(DraggableListCtrl* ctrl, long src, long tgt)
	: AtlasWindowCommand(true, _("Drag")), m_Ctrl(ctrl), m_Src(src), m_Tgt(tgt)
{
}

bool DragCommand::Do()
{
	wxASSERT(m_Tgt >= 0 && m_Src >= 0);

	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->MakeSizeAtLeast(m_Src+1);
	m_Ctrl->MakeSizeAtLeast(m_Tgt+1);

	// data[src] will be inserted just before data[tgt].
	// If tgt>src, (src+1)..tgt will be shifted down by 1.
	// If tgt<src, tgt..(src-1) will be shifted up by 1.

	AtObj srcData = m_Ctrl->m_ListData.at(m_Src);

	if (m_Tgt > m_Src)
	{
		std::copy(
				m_Ctrl->m_ListData.begin()+(m_Src+1),
				m_Ctrl->m_ListData.begin()+(m_Tgt+1),
				m_Ctrl->m_ListData.begin()+ m_Src);
	}
	else if (m_Tgt < m_Src)
	{
		std::copy_backward(
				m_Ctrl->m_ListData.begin()+ m_Tgt,
				m_Ctrl->m_ListData.begin()+ m_Src,
				m_Ctrl->m_ListData.begin()+(m_Src+1));
	}
	else // m_Tgt == m_Src
	{
		// do nothing - this item was just dragged onto itself
	}

	m_Ctrl->m_ListData.at(m_Tgt) = srcData;

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Tgt);

	return true;
}

bool DragCommand::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_Src);

	return true;
}

bool DragCommand::Merge(AtlasWindowCommand* command)
{
	DragCommand* previousCommand = wxDynamicCast(command, DragCommand);

	if (! previousCommand)
		return false;

	if (m_Src != previousCommand->m_Tgt)
		return false;

	previousCommand->m_Tgt = m_Tgt;
	return true;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(DeleteCommand, AtlasWindowCommand);

DeleteCommand::DeleteCommand(DraggableListCtrl* ctrl, long itemID)
	: AtlasWindowCommand(true, _("Delete")), m_Ctrl(ctrl), m_ItemID(itemID)
{
}

bool DeleteCommand::Do()
{
	wxASSERT(m_ItemID >= 0);

	// If we're asked to delete one of the blank rows off the end of the
	// list, don't do anything
	if (m_ItemID >= (long)m_Ctrl->m_ListData.size())
		return true;

	m_Ctrl->CloneListData(m_OldData);

	m_Ctrl->m_ListData.erase(m_Ctrl->m_ListData.begin()+m_ItemID);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_ItemID);

	return true;
}

bool DeleteCommand::Undo()
{
	m_Ctrl->SetListData(m_OldData);

	m_Ctrl->UpdateDisplay();
	m_Ctrl->SetSelection(m_ItemID);

	return true;
}
