#ifndef PMDCONVERT_H__
#define PMDCONVERT_H__

#include <string>

struct OutputCB;

void ColladaToPMD(const char* input, OutputCB& output, std::string& xmlErrors);

#endif // PMDCONVERT_H__
