#ifndef INCLUDED_PMDCONVERT
#define INCLUDED_PMDCONVERT

#include <string>

struct OutputCB;

void ColladaToPMD(const char* input, OutputCB& output, std::string& xmlErrors);

#endif // INCLUDED_PMDCONVERT
