#include "stdafx.h"

#include "Datafile.h"

#include "wx/filename.h"

AtObj Datafile::ReadList(const char* section)
{
	const wxString relativePath (_T("../data/tools/atlas/lists.xml"));

	wxFileName filename (relativePath, wxPATH_UNIX);
	filename.MakeAbsolute(systemDir);

	if (! filename.FileExists())
	{
		wxLogError(_("Cannot find file 'lists.xml'"));
		return AtObj();
	}

	AtObj lists (AtlasObject::LoadFromXML(filename.GetFullPath()));

	return lists["lists"][section];
}

void Datafile::SetSystemDirectory(const wxString& dir)
{
	systemDir = wxFileName(dir).GetPath();
}

wxString Datafile::systemDir;