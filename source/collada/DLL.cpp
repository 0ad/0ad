#include "precompiled.h"

#include "Converter.h"

#include <cstdarg>

void default_logger(int severity, const char* message)
{
	fprintf(stderr, "[%d] %s\n", severity, message);
}

static LogFn g_Logger = &default_logger;

void set_logger(LogFn logger)
{
	g_Logger = logger;
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

int convert_dae_to_pmd(const char* dae, OutputFn pmd_writer)
{
	Log(LOG_INFO, "Starting conversion");

	std::string xmlErrors;
	try
	{
		ColladaToPMD(dae, pmd_writer, xmlErrors);
	}
	catch (ColladaException e)
	{
		if (! xmlErrors.empty())
			Log(LOG_ERROR, "%s", xmlErrors.c_str());

		Log(LOG_ERROR, "%s", e.what());

		return -2;
	}

	if (! xmlErrors.empty())
	{
		Log(LOG_ERROR, "%s", xmlErrors.c_str());

		return -1;
	}

	return 0;
}
