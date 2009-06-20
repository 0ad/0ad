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

#include "wx/panel.h"

class QuickFileCtrl : public wxPanel
{
	DECLARE_DYNAMIC_CLASS(QuickFileCtrl);

public:
	QuickFileCtrl() {};
	QuickFileCtrl(wxWindow* parent, wxRect& location,
					const wxString& rootDir, const wxString& fileMask,
					wxString& rememberedDir,
					const wxValidator& validator = wxDefaultValidator);

	void OnKillFocus();

//private: // (or *theoretically* private)
	wxTextCtrl* m_TextCtrl;
	wxButton* m_ButtonBrowse;
	bool m_DisableKillFocus;

	wxString* m_RememberedDir; // can't be wxString&, because DYNAMIC_CLASSes need default constructors, and there's no suitable string to store in here...
};
