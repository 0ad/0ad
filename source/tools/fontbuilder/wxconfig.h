// $Id: wxconfig.h,v 1.2 2004/06/19 12:56:09 philip Exp $

#include "wx/string.h"

void ConfigInit();
wxString ConfigGet(wxString key);
void ConfigSet(wxString key, wxString value);
void ConfigDestroy();
