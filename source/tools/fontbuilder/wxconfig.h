#include "wx/string.h"

void ConfigInit();
wxString ConfigGet(wxString key);
void ConfigSet(wxString key, wxString value);
void ConfigDestroy();
