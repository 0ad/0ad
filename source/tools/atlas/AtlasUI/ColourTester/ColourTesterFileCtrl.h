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

#include "VirtualDirTreeCtrl/virtualdirtreectrl.h"

// wxGenericDirCtrl could potentially be used instead of this; but it gets
// indented a long way (since its root is far further back than necessary),
// and its icons aren't very pretty, and it'd be hard to adjust it to use VFS.

class ColourTesterImageCtrl;

class ColourTesterFileCtrl : public wxVirtualDirTreeCtrl
{
public:
	ColourTesterFileCtrl(wxWindow* parent, const wxSize& size, ColourTesterImageCtrl* imgctrl);

	virtual bool OnAddDirectory(VdtcTreeItemBase &item, const wxFileName &name);

private:
	void OnSelChanged(wxTreeEvent& event);

	ColourTesterImageCtrl* m_ImageCtrl;

	DECLARE_EVENT_TABLE();
};
