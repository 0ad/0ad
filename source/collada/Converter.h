#ifndef CONVERTER_H__
#define CONVERTER_H__

#include <exception>
#include <string>

class ColladaException : public std::exception
{
public:
	ColladaException(const std::string& msg)
		: std::exception(msg.c_str())
	{
	}
};

void ColladaToPMD(const char* input, OutputFn output, std::string& xmlErrors);

#endif // CONVERTER_H__
