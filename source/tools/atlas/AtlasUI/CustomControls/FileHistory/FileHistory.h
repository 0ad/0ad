#ifndef INCLUDED_FILEHISTORY
#define INCLUDED_FILEHISTORY

#include "wx/docview.h"

class FileHistory : public wxFileHistory
{
public:
	FileHistory(const wxString& configSubdir); // treated as relative - use "/AppName"
	virtual void Load(wxConfigBase& config);
	virtual void Save(wxConfigBase& config);

private:
	wxString m_configSubdir;
};

#endif // INCLUDED_FILEHISTORY
