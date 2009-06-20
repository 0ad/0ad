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

#include "ColourTesterFileCtrl.h"

#include "General/Datafile.h"
#include "ColourTesterImageCtrl.h"

BEGIN_EVENT_TABLE(ColourTesterFileCtrl, wxVirtualDirTreeCtrl)
	EVT_TREE_SEL_CHANGED(wxID_ANY, ColourTesterFileCtrl::OnSelChanged)
END_EVENT_TABLE()

ColourTesterFileCtrl::ColourTesterFileCtrl(wxWindow* parent, const wxSize& size, ColourTesterImageCtrl* imgctrl)
	: wxVirtualDirTreeCtrl(parent, wxID_ANY, wxDefaultPosition, size),
	m_ImageCtrl(imgctrl)
{
	wxFileName path (_T("mods/public/art/textures/skins/"));
	path.MakeAbsolute(Datafile::GetDataDirectory());
	wxASSERT(path.IsOk());
	SetRootPath(path.GetPath());
}

void ColourTesterFileCtrl::OnSelChanged(wxTreeEvent& event)
{
	if (IsFileNode(event.GetItem()))
	{
		m_ImageCtrl->SetImageFile(GetFullPath(event.GetItem()));
	}
}

bool ColourTesterFileCtrl::OnAddDirectory(VdtcTreeItemBase &item, const wxFileName &WXUNUSED(name))
{
	// Ignore .svn directories
	if (item.GetName() == _T(".svn"))
		return false;

	// Accept everything else
	return true;
}
