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

#ifndef INCLUDED_DRAGGABLELISTCTRL
#define INCLUDED_DRAGGABLELISTCTRL

/*
	DraggableListCtrl

	Based on wxListCtrl, but items can be reordered by dragging them around.
	Use just like a normal listctrl.
*/

#include "EditableListCtrl/EditableListCtrl.h"

#include "wx/listctrl.h"

class DragCommand;

class DraggableListCtrl : public EditableListCtrl
{
	friend class DragCommand;

public:
	DraggableListCtrl(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_ICON,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxListCtrlNameStr);

	void OnBeginDrag(wxListEvent& event);
	void OnEndDrag();

	void OnItemSelected(wxListEvent& event);
	void OnMouseEvent(wxMouseEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

private:
	long m_DragSource;

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_DRAGGABLELISTCTRL
