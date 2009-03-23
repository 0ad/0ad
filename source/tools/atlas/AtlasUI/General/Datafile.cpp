#include "precompiled.h"

#include "Datafile.h"

#include "wx/file.h"
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
