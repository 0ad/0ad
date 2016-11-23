/* Copyright (C) 2011 Wildfire Games.
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

#include "DraggableListCtrl.h"

#include "General/AtlasWindowCommandProc.h"
#include "DraggableListCtrlCommands.h"

const int ScrollSpeed = 8; // when dragging off the top or bottom of the control

DraggableListCtrl::DraggableListCtrl(wxWindow *parent,
									 wxWindowID id,
									 const wxPoint& pos,
									 const wxSize& size,
									 long style,
									 const wxValidator& validator,
									 const wxString& name)
	: EditableListCtrl(parent, id, pos, size, style, validator, name)
	, m_DragSource(0)
{
}

// TODO:
// * Dragging of multiple selections?

void DraggableListCtrl::OnBeginDrag(wxListEvent& WXUNUSED(event))
{
	CaptureMouse();
	SetFocus();
}

void DraggableListCtrl::OnEndDrag()
{
	AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
	commandProc->FinaliseLastCommand();
	SetSelection(m_DragSource);
	//commandProc->Store(new DragCommand(this, m_InitialDragSource, m_DragSource));
}

void DraggableListCtrl::OnItemSelected(wxListEvent& event)
{
	// Don't respond while in drag-mode - only the initial selection
	// (when starting the drag operation) should be handled
	if (! HasCapture())
	{
		// Remember which item is being dragged
		m_DragSource = event.GetIndex();

		// Make sure this listctrl is in focus
		SetFocus();
	}
}

void DraggableListCtrl::OnMouseCaptureLost(wxMouseCaptureLostEvent& WXUNUSED(event))
{
	// Mouse capture lost due to "external" event, like a dialog box or alt-tabbing
	//	(this is currently a Windows only event and failure to handle it will lead
	//	to an assertion failure in debug builds)
	OnEndDrag();
}

void DraggableListCtrl::OnMouseEvent(wxMouseEvent& event)
{
	// Only care when in drag-mode
	if (! HasCapture())
	{
		event.Skip();
		return;
	}

	if (event.LeftUp())
	{
		// Finished dragging; stop responding to mouse motion
		OnEndDrag();
		ReleaseMouse();
	}
	
	else if (event.Dragging())
	{
		// Find which item the mouse is now over
		int flags;
		long dragTarget = HitTest(event.GetPosition(), flags);

		if (dragTarget == wxNOT_FOUND)
		{
			// Not over an item. Scroll the view up/down if the mouse is
			// outside the listctrl.
			if (flags & wxLIST_HITTEST_ABOVE)
				ScrollList(0, -ScrollSpeed);
			else if (flags & wxLIST_HITTEST_BELOW)
				ScrollList(0, ScrollSpeed);
		}
		else
			if (flags & wxLIST_HITTEST_ONITEM && dragTarget != m_DragSource)
			{
				// Move the source item to the location under the mouse

				AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
				commandProc->Submit(new DragCommand(this, m_DragSource, dragTarget));

				// and remember that the source item is now in a different place
				m_DragSource = dragTarget;
			}
	}
	
	else
		// Some other kind of event which we're not interested in - ignore it
		event.Skip();
}

void DraggableListCtrl::OnChar(wxKeyEvent& event)
{
	// Don't respond to the keyboard if the user is dragging things (else
	// the undo system might get slightly confused)
	if (HasCapture())
		return;

	if (event.GetKeyCode() == WXK_DELETE)
	{
		long item = GetNextItem(-1,
				wxLIST_NEXT_ALL,
				wxLIST_STATE_SELECTED);

		if (item != -1)
		{
			AtlasWindowCommandProc::GetFromParentFrame(this)->Submit(
				new DeleteCommand(this, item)
			);
			UpdateDisplay();
		}
	}
	else
	{
		event.Skip();
	}
}


BEGIN_EVENT_TABLE(DraggableListCtrl, EditableListCtrl)
	EVT_LIST_BEGIN_DRAG(wxID_ANY, DraggableListCtrl::OnBeginDrag)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, DraggableListCtrl::OnItemSelected)
	EVT_MOTION(DraggableListCtrl::OnMouseEvent)
	EVT_LEFT_UP(DraggableListCtrl::OnMouseEvent)
	EVT_CHAR(DraggableListCtrl::OnChar)
	EVT_MOUSE_CAPTURE_LOST(DraggableListCtrl::OnMouseCaptureLost)
END_EVENT_TABLE()
