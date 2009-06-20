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

#ifndef INCLUDED_EDITABLELISTCTRL
#define INCLUDED_EDITABLELISTCTRL

#include "wx/listctrl.h"

#include "General/IAtlasSerialiser.h"

#include <vector>

class FieldEditCtrl;
class AtObj;
class AtIter;

class EditableListCtrl : public wxListCtrl, public IAtlasSerialiser
{
	friend class DeleteCommand;
	friend class DragCommand;
	friend class ImportCommand;
	friend class PasteCommand;

public:
	EditableListCtrl(wxWindow *parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxLC_ICON,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxListCtrlNameStr);

	~EditableListCtrl();

	void OnKeyDown(wxKeyEvent& event);
	void OnMouseEvent(wxMouseEvent& event);

	void MakeSizeAtLeast(int n);

	long GetSelection();
	void SetSelection(long item);

	void UpdateDisplay();

	wxString GetCellString(long item, long column) const;
	AtObj GetCellObject(long item, long column) const;
	void SetCellString(long item, long column, wxString& str);
	void SetCellObject(long item, long column, AtObj& obj);

	struct ColumnData
	{
		ColumnData(const char* k, const FieldEditCtrl* c) : key(k), ctrl(c) {}
		const char* key;
		const FieldEditCtrl* ctrl;
	};
	std::vector<ColumnData> m_ColumnTypes;

	void CloneListData(std::vector<AtObj>& out);
	void SetListData(std::vector<AtObj>& in);

	void DeleteData();


	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

private:
	int GetColumnAtPosition(wxPoint& pos);
	void GetCellRect(long row, int col, wxRect& rect);

	void TrimBlankEnds();

	wxString OnGetItemText(long item, long column) const;

protected:

	wxListItemAttr* OnGetItemAttr(long item) const;

	virtual void DoImport(AtObj&)=0;
	virtual AtObj DoExport()=0;

	std::vector<AtObj> m_ListData;

	// objectkey must remain in existence for as long as this list control
	// exists (so you really don't want to be dynamically allocating it;
	// just use static constant strings)
	void AddColumnType(const wxString& title, int width, const char* objectkey, FieldEditCtrl* ctrl);

	void AddRow(AtObj& obj);
	void AddRow(AtIter& iter);

	bool IsRowBlank(int n);

	wxListItemAttr m_ListItemAttr[2]; // standard+alternate colours

	DECLARE_EVENT_TABLE();
};

#endif // INCLUDED_EDITABLELISTCTRL
