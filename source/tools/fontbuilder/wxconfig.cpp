// $Id: wxconfig.cpp,v 1.2 2004/06/19 12:56:09 philip Exp $

#include "stdafx.h"

#include "wx/config.h"
#include "wx/filename.h"

#include "wxconfig.h"

void ConfigInit()
{
	wxConfig* cfg = new wxConfig(wxT("WFG Font Builder"));
	wxConfig::Set(cfg);

	// Default paths, for the first time program is run:

	// Get "x:\wherever\etc\binaries\"
	wxFileName cwd = wxFileName::GetCwd()+wxT("\\");
	cwd.RemoveDir((int)cwd.GetDirCount()-1);

	if (!ConfigGet(wxT("FSF path")))
	{
		wxFileName dir = cwd; dir.AppendDir(wxT("data\\tools\\fontbuilder\\settings"));
		ConfigSet(wxT("FSF path"), dir.GetPath(wxPATH_GET_VOLUME));
	}

	if (!ConfigGet(wxT("FNT path")))
	{
		wxFileName dir = cwd; dir.AppendDir(wxT("data\\mods\\official\\fonts"));
		ConfigSet(wxT("FNT path"), dir.GetPath(wxPATH_GET_VOLUME));
	}

	if (!ConfigGet(wxT("Charset path")))
	{
		wxFileName dir = cwd; dir.AppendDir(wxT("data\\tools\\fontbuilder\\charsets"));
		ConfigSet(wxT("Charset path"), dir.GetPath(wxPATH_GET_VOLUME));
	}
}

wxString ConfigGet(wxString key)
{
	wxConfig* cfg = (wxConfig*) wxConfig::Get();
	wxString ret;
	cfg->Read(key, &ret);
	return ret;
}

void ConfigSet(wxString key, wxString value)
{
	wxConfig* cfg = (wxConfig*) wxConfig::Get();
	cfg->Write(key, value);
}

void ConfigDestroy()
{
	delete wxConfig::Get();
}

