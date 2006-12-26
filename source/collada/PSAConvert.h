#ifndef PSACONVERT_H__
#define PSACONVERT_H__

#include <string>

struct OutputCB;

void ColladaToPSA(const char* input, OutputCB& output, std::string& xmlErrors);

#endif // PSACONVERT_H__
