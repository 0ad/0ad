// $Id: filemanip.h,v 1.1 2004/06/17 19:32:04 philip Exp $

#include <set>

void RGB_OutputGreyscaleTGA(unsigned char* image_data, int width, int height, int pitch, wxFFile& file);
std::set<wchar_t> AnalyseChars(wxString filename);
