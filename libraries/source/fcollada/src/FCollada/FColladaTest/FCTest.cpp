/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"

#ifdef WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif // WIN32

fstring logFilename = FC("FColladaTestLog.txt");
bool isVerbose = true;

void ShowHelp()
{
	fprintf(stderr, "Unknown command line argument(s). \n");
	fprintf(stderr, "Usage: FColladaTest.exe [-v <is verbose?>] [<filename>]\n");
	fprintf(stderr, "  -v <is verbose?>: Set the verbosity level.\n");
	fprintf(stderr, "      Level 0 keeps only the error messages.\n");
	fprintf(stderr, "      Level 1 keeps all the messages. \n");
	fprintf(stderr, " <filename>: Set the filename of the output log file.\n");
	fflush(stderr);
	exit(-1);
}

void ProcessCommandLine(int argc, char* argv[])
{
	// Remove the first argument: the current application name...
	--argc; ++argv;
	while (argc > 0)
	{
		if (argv[0][0] == '-' || argv[0][0] == '/')
		{
			++(argv[0]);
			if (strcmp(argv[0], "-v") && argc >= 2)
			{
				isVerbose = FUStringConversion::ToBoolean(argv[1]);
				argc -= 2; argv += 2;
			}
			else
			{
				// unknown flag
				ShowHelp(); break;
			}
		}
		else if (argc == 1)
		{
			logFilename = TO_FSTRING((const char*) *(argv++));
			--argc;
		}
		else
		{
			ShowHelp(); break;
		}
	}
}

int main(int argc, char* argv[])
{
	ProcessCommandLine(argc, argv);
	FCollada::Initialize(); //Needed for Mac/Linux when FCollada is statically linked.
	// In debug mode, output memory leak information and do periodic memory checks
#ifdef PLUG_CRT
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_128_DF);
#endif

	FUTestBed testBed(logFilename.c_str(), isVerbose);

#ifndef _DEBUG
	try {
#endif
	// Set the current folder to the folder with the samples DAE files
	_chdir("Samples");

	// FMath tests
	RUN_TESTSUITE(FCTestAll)

#ifndef _DEBUG
	}
	catch (const char* sz) { testBed.GetLogFile().WriteLine(sz); }
#ifdef UNICODE
	catch (const fchar* sz) { testBed.GetLogFile().WriteLine(sz); }
#endif
	catch (...) { testBed.GetLogFile().WriteLine("Exception caught!"); }
#endif

	return 0;
}

TESTSUITE_START(FCTestAll)

TESTSUITE_TEST(0, FColladaAll)
	// FCollada internal tests.
	FCollada::RunTests(testBed);

	// FCDocument test
	RUN_TESTSUITE(FCDParameter);
	RUN_TESTSUITE(FColladaArchiving);
	RUN_TESTSUITE(FCDAnimation);
	RUN_TESTSUITE(FCDGeometryPolygonsTools);
	RUN_TESTSUITE(FCDExportReimport);
	RUN_TESTSUITE(FCTestXRef);
	RUN_TESTSUITE(FCTAssetManagement);
	RUN_TESTSUITE(FCDControllers);
	RUN_TESTSUITE(FCDSceneNode);

TESTSUITE_END
