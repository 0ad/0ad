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

#include "Windows/AtlasDialog.h"

#include "DraggableListCtrl/DraggableListCtrl.h"

class AnimListEditorListCtrl;

//////////////////////////////////////////////////////////////////////////

class AnimListEditor : public AtlasDialog
{
public:
	AnimListEditor(wxWindow* parent);
	static AtlasDialog* Create(wxWindow* parent) { return new AnimListEditor(parent); }

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

private:
	AnimListEditorListCtrl* m_MainListBox;
};

//////////////////////////////////////////////////////////////////////////

class AnimListEditorListCtrl : public DraggableListCtrl
{
	friend class AnimListEditor;

public:
	AnimListEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	void DoImport(AtObj& in);
	AtObj DoExport();
};
