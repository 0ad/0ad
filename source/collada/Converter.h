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

struct OutputCB
{
	virtual void operator() (const char* data, unsigned int length)=0;
};

void ColladaToPMD(const char* input, OutputCB& output, std::string& xmlErrors);

#endif // CONVERTER_H__
