#ifndef FILEHISTORY_H__
#define FILEHISTORY_H__

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

#endif // FILEHISTORY_H__
