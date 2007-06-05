#include "precompiled.h"

#include "FileHistory.h"

#include "wx/confbase.h"

FileHistory::FileHistory(const wxString& configSubdir)
: wxFileHistory(9), m_configSubdir(configSubdir)
{
}

void FileHistory::Load(wxConfigBase& config)
{
	wxString old = config.GetPath();
	config.SetPath(m_configSubdir);
	wxFileHistory::Load(config);
	config.SetPath(old);
}

void FileHistory::Save(wxConfigBase& config)
{
	wxString old = config.GetPath();
	config.SetPath(m_configSubdir);
	wxFileHistory::Save(config);
	config.SetPath(old);
}
