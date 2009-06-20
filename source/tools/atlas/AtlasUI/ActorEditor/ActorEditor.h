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

#include "Windows/AtlasWindow.h"

class ActorEditorListCtrl;

class ActorEditor : public AtlasWindow
{
	enum
	{
		ID_CreateEntity = ID_Custom
	};

public:
	ActorEditor(wxWindow* parent);

private:
	void OnCreateEntity(wxCommandEvent& event);

protected:
	AtObj FreezeData();
	void ThawData(AtObj& in);

	AtObj ExportData();
	void ImportData(AtObj& in);

	// TODO: er, what's the difference between freeze/thaw and export/import?
	// Why is the code duplicated between them?

	virtual wxString GetDefaultOpenDirectory();

private:
	ActorEditorListCtrl* m_ActorEditorListCtrl;

	wxCheckBox* m_CastShadows;
	wxCheckBox* m_Float;
	wxComboBox* m_Material;

	DECLARE_EVENT_TABLE();
};
