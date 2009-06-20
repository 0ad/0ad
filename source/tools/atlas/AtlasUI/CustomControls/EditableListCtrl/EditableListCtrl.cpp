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

#include "EditableListCtrl.h"

#include "EditableListCtrlCommands.h"
#include "FieldEditCtrl.h"
#include "General/AtlasWindowCommandProc.h"
#include "AtlasObject/AtlasObject.h"
#include "AtlasObject/AtlasObjectText.h"
#include "General/AtlasClipboard.h"

const int BlanksAtEnd = 2;

EditableListCtrl::EditableListCtrl(wxWindow *parent,
								   wxWindowID id,
								   const wxPoint& pos,
								   const wxSize& size,
								   long style,
								   const wxValidator& validator,
								   const wxString& name)
	: wxListCtrl(parent, id, pos, size, style | wxLC_VIRTUAL, validator, name)
{
	m_ListItemAttr[0].SetBackgroundColour(wxColour(0xff, 0xff, 0xff));
	m_ListItemAttr[1].SetBackgroundColour(wxColour(0xee, 0xee, 0xee));

	wxASSERT_MSG(style & wxLC_REPORT, _T("EditableListCtrl must be LC_REPORT"));
	UpdateDisplay();
}

EditableListCtrl::~EditableListCtrl()
{
	size_t count = m_ColumnTypes.size();
	for (size_t n = 0; n < count; ++n)
		delete (FieldEditCtrl*)m_ColumnTypes[n].ctrl;

	m_ColumnTypes.clear();
}

void EditableListCtrl::AddColumnType(const wxString& title, int width, const char* objectkey, FieldEditCtrl* ctrl)
{
	int n = GetColumnCount();
	wxASSERT(m_ColumnTypes.size() == (size_t) n); // check internal consistency

	InsertColumn(n, title, wxLIST_FORMAT_LEFT, width);

	m_ColumnTypes.push_back(ColumnData(objectkey, ctrl));
}


void EditableListCtrl::OnMouseEvent(wxMouseEvent& event)
{
	// Double-clicking/right-clicking on a cell lets the user edit it.
	// The editing method depends on what column the cell is in.

	if (event.LeftDClick() || event.RightDown())
	{
		// Work out what cell was clicked on:

		wxPoint pt = event.GetPosition();
		int col = GetColumnAtPosition(pt);

		wxCHECK2(col >= 0 && col < (int)m_ColumnTypes.size(), return);

		int flags;
		long row = HitTest(pt, flags);

		if (row != wxNOT_FOUND && (flags & wxLIST_HITTEST_ONITEM))
		{
			// Calculate the exact positioning of the clicked cell
			wxRect rect;
			GetCellRect(row, col, rect);

			// Execute the appropriate FieldEditCtrl
			FieldEditCtrl* editor = (FieldEditCtrl*)m_ColumnTypes[col].ctrl;
			editor->StartEdit(this, rect, row, col);
		}
	}
}

void EditableListCtrl::OnKeyDown(wxKeyEvent& event)
{
	// TODO: Don't use magic key-code numbers

	// Check for Copy
	if ((event.GetKeyCode() == 3) || // ctrl+c
		(event.GetKeyCode() == WXK_INSERT && event.ControlDown())) // ctrl+insert
	{
		AtObj row;
		long selection = GetSelection();
		if (selection >= 0 && selection < (long)m_ListData.size())
			row = m_ListData[selection];
		AtlasClipboard::SetClipboard(row);
	}

	// Check for Paste
	else
	if ((event.GetKeyCode() == 22) || // ctrl+v
		(event.GetKeyCode() == WXK_INSERT && event.ShiftDown())) // shift+insert
	{
		AtObj row;
		if (AtlasClipboard::GetClipboard(row))
		{
			long selection = GetSelection();
			AtlasWindowCommandProc* commandProc = AtlasWindowCommandProc::GetFromParentFrame(this);
			commandProc->Submit(new PasteCommand(this, selection, row));
		}
	}
	else
		event.Skip();
}

int EditableListCtrl::GetColumnAtPosition(wxPoint& pos)
{
	// Find the column which pos is in.

	// Get the origin of the table, in case it's scrolled horizontally
	wxRect rect;
	GetItemRect(0, rect);
	int x = rect.GetX();

	// Loop through each column
	int numCols = GetColumnCount();
	for (int i = 0; i < numCols; ++i)
	{
		// Calculate the position of this column's right-hand edge
		x += GetColumnWidth(i);

		// Test if pos was within this column (and assume it wasn't in an earlier one)
		if (pos.x <= x)
			return i;
	}

	// Point is outside the table's right edge
	return -1;
}

void EditableListCtrl::GetCellRect(long row, int col, wxRect& rect)
{
	wxASSERT(col >= 0 && col < GetColumnCount());
	wxASSERT(row >= 0 && row < GetItemCount());

	GetItemRect(row, rect);

	for (int i = 0; i < col; ++i)
		rect.x += GetColumnWidth(i);

	rect.width = GetColumnWidth(col);
}


bool EditableListCtrl::IsRowBlank(int n)
{
	return ! m_ListData[n].hasContent();
}

void EditableListCtrl::TrimBlankEnds()
{
	while (m_ListData.size() && !m_ListData.back().defined())
		m_ListData.pop_back();
}


void EditableListCtrl::SetSelection(long item)
{
	SetItemState(item, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
}

long EditableListCtrl::GetSelection()
{
	for (long item = 0; item < GetItemCount(); ++item)
		if (GetItemState(item, wxLIST_STATE_SELECTED))
			return item;
	return 0;
}


void EditableListCtrl::MakeSizeAtLeast(int n)
{
	if ((int)m_ListData.size() < n)
		m_ListData.resize(n);
}

void EditableListCtrl::AddRow(AtObj& obj)
{
	m_ListData.push_back(obj);
}

void EditableListCtrl::AddRow(AtIter& iter)
{
	AtObj obj = *iter;
	AddRow(obj);
}

void EditableListCtrl::UpdateDisplay()
{
	TrimBlankEnds();
	SetItemCount((int)m_ListData.size() + BlanksAtEnd);
	Refresh();
}


void EditableListCtrl::CloneListData(std::vector<AtObj>& out)
{
	out = m_ListData;
}

void EditableListCtrl::SetListData(std::vector<AtObj>& in)
{
	m_ListData = in;
}

void EditableListCtrl::DeleteData()
{
	m_ListData.clear();
}

wxString EditableListCtrl::GetCellString(long item, long column) const
{
	wxCHECK(item >= 0 && column >= 0 && column < (int)m_ColumnTypes.size(), _T(""));

	if (item >= (int)m_ListData.size())
		return _T("");

	AtObj cell = *m_ListData[item][m_ColumnTypes[column].key];
	return AtlasObject::ConvertToString(cell).c_str();
}

AtObj EditableListCtrl::GetCellObject(long item, long column) const
{
	wxCHECK(item >= 0 && column >= 0 && column < (int)m_ColumnTypes.size(), AtObj());

	if (item >= (int)m_ListData.size())
		return AtObj();

	return *m_ListData[item][m_ColumnTypes[column].key];
}

void EditableListCtrl::SetCellString(long item, long column, wxString& str)
{
	wxCHECK(item >= 0 && column >= 0 && column < (int)m_ColumnTypes.size(), );
	MakeSizeAtLeast(item+1);
	m_ListData[item].set(m_ColumnTypes[column].key, str.c_str());
}

void EditableListCtrl::SetCellObject(long item, long column, AtObj& obj)
{
	wxCHECK(item >= 0 && column >= 0 && column < (int)m_ColumnTypes.size(), );
	MakeSizeAtLeast(item+1);
	m_ListData[item].set(m_ColumnTypes[column].key, obj);
}

wxString EditableListCtrl::OnGetItemText(long item, long column) const
{
	return GetCellString(item, column);
}

wxListItemAttr* EditableListCtrl::OnGetItemAttr(long item) const
{
	// Make the last two rows white
	if (item >= (long)m_ListData.size())
		return const_cast<wxListItemAttr*>(&m_ListItemAttr[0]);

	// Make the background colours of rows alternate
	else
		return const_cast<wxListItemAttr*>(&m_ListItemAttr[item%2]);
}

void EditableListCtrl::ImportData(AtObj& in)
{
	return DoImport(in);
}

AtObj EditableListCtrl::ExportData()
{
	return DoExport();
}


void EditableListCtrl::ThawData(AtObj& in)
{
	m_ListData.clear();
	for (AtIter it = in["item"]; it.defined(); ++it)
		m_ListData.push_back(*it);
	UpdateDisplay();
}

AtObj EditableListCtrl::FreezeData()
{
	AtObj out;
	for (std::vector<AtObj>::iterator it = m_ListData.begin(); it != m_ListData.end(); ++it)
		out.add("item", *it);
	return out;
}



BEGIN_EVENT_TABLE(EditableListCtrl, wxListCtrl)
	EVT_LEFT_DCLICK(EditableListCtrl::OnMouseEvent)
	EVT_RIGHT_DOWN(EditableListCtrl::OnMouseEvent)
	EVT_CHAR(EditableListCtrl::OnKeyDown)
END_EVENT_TABLE()
