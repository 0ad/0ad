/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_FILEHISTORY
#define INCLUDED_FILEHISTORY

#include "wx/docview.h"

class FileHistory : public wxFileHistory
{
public:
	FileHistory(const wxString& configSubdir); // treated as relative - use "/AppName"
	virtual void LoadFromSubDir(wxConfigBase& config);
	virtual void SaveToSubDir(wxConfigBase& config);

private:
#if wxCHECK_VERSION(2, 9, 0)
	virtual void Load(const wxConfigBase& config)
#else
	virtual void Load(wxConfigBase& config)
#endif
	{
		wxFileHistory::Load(config);
	}

	virtual void Save(wxConfigBase& config)
	{
		wxFileHistory::Save(config);
	}

	wxString m_configSubdir;
};

#endif // INCLUDED_FILEHISTORY
