// $Id: wxconfig.h,v 1.1 2004/06/17 19:32:04 philip Exp $

void ConfigInit();
wxString ConfigGet(wxString key);
void ConfigSet(wxString key, wxString value);
void ConfigDestroy();
