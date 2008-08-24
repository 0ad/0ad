#include "precompiled.h"

#include "CommonConvert.h"
#include "PMDConvert.h"
#include "PSAConvert.h"
#include "StdSkeletons.h"

#include <cstdarg>
#include <cassert>

void default_logger(int severity, const char* message)
{
	fprintf(stderr, "[%d] %s\n", severity, message);
}

static LogFn g_Logger = &default_logger;

EXPORT void set_logger(LogFn logger)
{
	if (logger)
		g_Logger = logger;
	else
		g_Logger = &default_logger;
}

void Log(int severity, const char* msg, ...)
{
	char buffer[1024];
	va_list ap;
	va_start(ap, msg);
	vsnprintf(buffer, sizeof(buffer), msg, ap);
	buffer[sizeof(buffer)-1] = '\0';
	va_end(ap);

	g_Logger(severity, buffer);
}

struct BufferedOutputCallback : public OutputCB
{
	static const int bufferSize = 4096;
	char buffer[bufferSize];
	int bufferUsed;

	OutputFn fn;
	void* cb_data;

	BufferedOutputCallback(OutputFn fn, void* cb_data)
		: fn(fn), cb_data(cb_data), bufferUsed(0)
	{
	}

	~BufferedOutputCallback()
	{
		// flush the buffer if it's not empty
		if (bufferUsed > 0)
			fn(cb_data, buffer, bufferUsed);
	}

	virtual void operator() (const char* data, unsigned int length)
	{
		if (bufferUsed+length > bufferSize)
		{
			// will overflow buffer, so flush the buffer first
			fn(cb_data, buffer, bufferUsed);
			bufferUsed = 0;

			if (length > bufferSize)
			{
				// new data won't fit in buffer, so send it out unbuffered
				fn(cb_data, data, length);
				return;
			}
		}

		// append onto buffer
		memcpy(buffer+bufferUsed, data, length);
		bufferUsed += length;
		assert(bufferUsed <= bufferSize);
	}
};

int convert_dae_to_whatever(const char* dae, OutputFn writer, void* cb_data, void(*conv)(const char*, OutputCB&, std::string&))
{
	Log(LOG_INFO, "Starting conversion");

	FCollada::Initialize();

	std::string xmlErrors;
	BufferedOutputCallback cb(writer, cb_data);
	try
	{
		conv(dae, cb, xmlErrors);
	}
	catch (const ColladaException& e)
	{
		if (! xmlErrors.empty())
			Log(LOG_ERROR, "%s", xmlErrors.c_str());

		Log(LOG_ERROR, "%s", e.what());

		FCollada::Release();

		return -2;
	}

	FCollada::Release();

	if (! xmlErrors.empty())
	{
		Log(LOG_ERROR, "%s", xmlErrors.c_str());

		return -1;
	}

	return 0;
}

EXPORT int convert_dae_to_pmd(const char* dae, OutputFn pmd_writer, void* cb_data)
{
	return convert_dae_to_whatever(dae, pmd_writer, cb_data, ColladaToPMD);
}

EXPORT int convert_dae_to_psa(const char* dae, OutputFn psa_writer, void* cb_data)
{
	return convert_dae_to_whatever(dae, psa_writer, cb_data, ColladaToPSA);
}

EXPORT int set_skeleton_definitions(const char* xml, int length)
{
	std::string xmlErrors;
	try
	{
		Skeleton::LoadSkeletonDataFromXml(xml, length, xmlErrors);
	}
	catch (const ColladaException& e)
	{
		if (! xmlErrors.empty())
			Log(LOG_ERROR, "%s", xmlErrors.c_str());

		Log(LOG_ERROR, "%s", e.what());

		return -1;
	}

	return 0;
}
