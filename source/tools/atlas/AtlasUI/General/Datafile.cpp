#include "stdafx.h"

#include "Datafile.h"

#include "wx/filename.h"
#include "wx/dir.h"

static wxString systemDir;
static wxString dataDir;

AtObj Datafile::ReadList(const char* section)
{
	const wxString relativePath (_T("tools/atlas/lists.xml"));

	wxFileName filename (relativePath, wxPATH_UNIX);
	filename.MakeAbsolute(dataDir);

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
	wxFileName sys (dir);
	systemDir = sys.GetPath();

	wxFileName data (_T("../data/"), wxPATH_UNIX);
	data.MakeAbsolute(systemDir);
	dataDir = data.GetPath();
}

wxString Datafile::GetDataDirectory()
{
	return dataDir;
}

wxArrayString Datafile::EnumerateDataFiles(const wxString& dir, const wxString& filter)
{
	wxFileName d (dir);
	d.MakeAbsolute(dataDir);

	wxArrayString files;
	wxDir::GetAllFiles(d.GetPath(), &files, filter, wxDIR_FILES);
	return files;
}
