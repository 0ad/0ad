/*
Config.h

CConfig dynamic data file manager class
Mark Thompson (mot20@cam.ac.uk)

Last modified: 22 November 2003 (Mark Thompson)

--Overview--

Maintains a list of data files in use by the engine; reloads any data file 
altered during execution when Update() is called.

--Usage--

Load files by calling CConfig::Register() for each. The files are not loaded
until Update() is called; do so at the end of the block.

Update() compares the 'last-modified' timestamps of all registered files,
reloading all that have been altered since the last call (also any newly
registered).

Loader functions passed to Register() must take a CStr argument for a
filename, a void* for additional data, and return PS_OK if successful.
They must also handle the case where modified data is being reloaded. Ideally,
they should release resources allocated to the old version and load the new, at
least functions must leave the system in a predictable state. (e.g. if graphics
files are changed, but the loader functions cannot reload them, they should
do nothing but return PS_FILE_NODYNAMIC)

--Examples--

g_Config.Register( "gameParameters.dat", NULL, paramLoader );
g_Config.Register( "graphicsParameters7.dat", (void*)7, gfxParamLoader );
g_Config.Update();

--More info--

TDD at http://forums.wildfiregames.com/0ad

*/

#ifndef Config_H
#define Config_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "stdlib.h"
#include "Prometheus.h"
#include "Singleton.h"
#include "CStr.h"
#include "LogFile.h"
#include "posix.h"
#include "zip.h"
#include "misc.h"

#include <vector>

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

// Get singleton
#define g_Config CConfig::GetSingleton()

//Dummy timestamp value
#define TIME_UNREGISTERED 0
#define ACCESS_EXISTS 0
//The maximum number of files processed in one call to Update()
#define CONFIG_SLICE 100		

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

//Possible return codes
DECLARE_ERROR( PS_FILE_NOT_FOUND );
DECLARE_ERROR( PS_FILE_LOAD_FAILURE );
DECLARE_ERROR( PS_FILE_NODYNAMIC );

//Loader function
typedef PS_RESULT (*LoaderFunction)( CStr Filename, void* Data );

//Internal registration type
struct SConfigData
{
	CStr Filename;
	LoaderFunction DynamicLoader;
	void* Data;
	bool Static;
	time_t Timestamp;
	SConfigData( CStr _Filename, void* _Data, LoaderFunction _DynamicLoader, bool _Static );
};

class CConfig : public Singleton<CConfig>
{
public:
	CConfig();
	//Register a new file with it's associated loader function
	PS_RESULT Register( CStr Filename, void* Data, LoaderFunction DynamicLoader, bool Static = false );
	//Check all registered files, reload as neccessary
	PS_RESULT Update();
	//Force an update of all files in the registered and static lists.
	PS_RESULT ReloadAll();
	//Erase the entire list of registered and static files
	void Clear();
	//Attach or detach a logfile class.
	void Attach( CLogFile* LogFile );
private:
	vector<SConfigData> m_FileList;
	vector<SConfigData>::iterator i;
	CLogFile* m_LogFile;
};

#endif
