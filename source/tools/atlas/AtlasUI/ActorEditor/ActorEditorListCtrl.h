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

#include "DraggableListCtrl/DraggableListCtrl.h"

class ActorEditor;

class ActorEditorListCtrl : public DraggableListCtrl
{
	friend class ActorEditor;

public:
	ActorEditorListCtrl(wxWindow* parent);

	void OnUpdate(wxCommandEvent& event);

	wxListItemAttr* OnGetItemAttr(long item) const;

private:
	void DoImport(AtObj& in);
	AtObj DoExport();

	wxListItemAttr m_ListItemAttr_Model[2];
	wxListItemAttr m_ListItemAttr_Texture[2];
	wxListItemAttr m_ListItemAttr_Anim[2];
	wxListItemAttr m_ListItemAttr_Prop[2];
	wxListItemAttr m_ListItemAttr_Colour[2];
	wxListItemAttr m_ListItemAttr_None[2];
};
