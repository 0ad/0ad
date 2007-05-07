#ifndef INCLUDED_PSACONVERT
#define INCLUDED_PSACONVERT

#include <string>

struct OutputCB;

void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors);

#endif // INCLUDED_PSACONVERT
