#include "wx/ffile.h"

#include <set>

void RGB_OutputGreyscaleTGA(unsigned char* image_data, int width, int height, int pitch, wxFFile& file);
std::set<wchar_t> AnalyseChars(wxString filename);
