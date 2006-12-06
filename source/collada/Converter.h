#ifndef CONVERTER_H__
#define CONVERTER_H__

#include <exception>
#include <string>

class ColladaException : public std::exception
{
public:
	ColladaException(const std::string& msg)
		: msg(msg)
	{
	}

	~ColladaException() throw()
	{
	}

	virtual const char* what() const throw()
	{
		return msg.c_str();
	}
private:
	std::string msg;
};

void ColladaToPMD(const char* input, OutputFn output, std::string& xmlErrors);

#endif // CONVERTER_H__
