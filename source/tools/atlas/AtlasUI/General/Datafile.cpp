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

#include "Datafile.h"

#include "wx/file.h"
#include "wx/filename.h"
#include "wx/dir.h"

static wxString g_DataDir;

AtObj Datafile::ReadList(const char* section)
{
	const wxString relativePath (_T("tools/atlas/lists.xml"));

	wxFileName filename (relativePath, wxPATH_UNIX);
	filename.MakeAbsolute(g_DataDir);

	if (! filename.FileExists())
	{
		wxLogError(_("Cannot find file 'lists.xml'"));
		return AtObj();
	}

	std::string xml;
	wxCHECK(SlurpFile(filename.GetFullPath(), xml), AtObj());
	AtObj lists (AtlasObject::LoadFromXML(xml));

	return *lists["lists"][section];
}

bool Datafile::SlurpFile(const wxString& filename, std::string& out)
{
	wxFile file (filename, wxFile::read);
	while (true)
	{
		static char buf[1024];
		int read = file.Read(buf, sizeof(buf));
		wxCHECK(read >= 0, false);
		if (read == 0)
			break;
		out += std::string(buf, read);
	}
	file.Close();
	return true;
}

void Datafile::SetSystemDirectory(const wxString& dir)
{
	wxFileName sys (dir);
	wxString systemDir = sys.GetPath();

	wxFileName data (_T("../data/"), wxPATH_UNIX);
	data.MakeAbsolute(systemDir);
	g_DataDir = data.GetPath();
}

void Datafile::SetDataDirectory(const wxString& dir)
{
	wxFileName data (dir);
	g_DataDir = data.GetPath();
}

wxString Datafile::GetDataDirectory()
{
	return g_DataDir;
}

wxArrayString Datafile::EnumerateDataFiles(const wxString& dir, const wxString& filter)
{
	wxFileName d (dir);
	d.MakeAbsolute(g_DataDir);

	wxArrayString files;
	wxDir::GetAllFiles(d.GetPath(), &files, filter, wxDIR_FILES);
	return files;
}
