// $Id: wxconfig.cpp,v 1.3 2004/06/19 13:46:11 philip Exp $

#include "stdafx.h"

#include "wx/config.h"
#include "wx/filename.h"

#include "wxconfig.h"

#ifdef _WIN32
const wxString PathSep = wxT("\\");
#else
const wxString PathSep = wxT("/");
#endif

void ConfigInit()
{
	wxConfig* cfg = new wxConfig(wxT("WFG Font Builder"));
	wxConfig::Set(cfg);

	// Default paths, for the first time program is run:

	// Get "x:\wherever\etc\binaries\"
	wxFileName cwd = wxFileName::GetCwd()+PathSep;
	cwd.RemoveDir((int)cwd.GetDirCount()-1);

#define DIR(a) dir.AppendDir(wxT(a))

	cwd.AppendDir(wxT("data"));
	if (!ConfigGet(wxT("FSF path")))
	{
		wxFileName dir = cwd; 
		DIR("tools"); DIR("fontbuilder"); DIR("settings");
		ConfigSet(wxT("FSF path"), dir.GetPath(wxPATH_GET_VOLUME));
	}

	if (!ConfigGet(wxT("FNT path")))
	{
		wxFileName dir = cwd;
		DIR("mods"); DIR("official"); DIR("fonts");
		ConfigSet(wxT("FNT path"), dir.GetPath(wxPATH_GET_VOLUME));
	}

	if (!ConfigGet(wxT("Charset path")))
	{
		wxFileName dir = cwd;
		DIR("tools"); DIR("fontbuilder"); DIR("charsets");
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

