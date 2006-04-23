#include "precompiled.h"

#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CConsole.h"
#include "lib/timer.h"
#include "lib/res/sound/snd_mgr.h"
#include "lib/res/file/trace.h"
#include "lib/res/file/vfs_optimizer.h"
#include "Config.h"

#define LOG_CATEGORY "config"

CStr g_CursorName = "test";
CStr g_ActiveProfile = "default";

// flag to disable extended GL extensions until fix found - specifically, crashes
// using VBOs on laptop Radeon cards
bool g_NoGLVBO=false;
// disable FBO extension in case the driver is flaky
bool g_NoGLFramebufferObject = false;
// flag to switch on shadows
bool g_Shadows=false;
// flag to switch on fixed frame timing (RC: I'm using this for profiling purposes)
bool g_FixedFrameTiming=false;
bool g_VSync = false;
float g_LodBias = 0.0f;
float g_Gamma = 1.0f;
bool g_EntGraph = false;
CStr g_RenderPath = "default";

// graphics mode
int g_xres, g_yres;
int g_bpp;
int g_freq;

bool g_Quickstart=false;

// If non-empty, specified map will be automatically loaded
CStr g_AutostartMap = "";

//----------------------------------------------------------------------------
// config and profile
//----------------------------------------------------------------------------

static void LoadProfile( CStr profile )
{
	CStr base = CStr( "profiles/" ) + profile;
	g_ConfigDB.SetConfigFile(CFG_USER, true, base +  "/settings/user.cfg");
	g_ConfigDB.Reload(CFG_USER);

	int max_history_lines = 50;
	CFG_GET_USER_VAL("console.history.size", Int, max_history_lines);
	g_Console->UseHistoryFile(base+"/settings/history", max_history_lines);
}


// Fill in the globals from the config files.
static void LoadGlobals()
{
	CFG_GET_SYS_VAL("profile", String, g_ActiveProfile);

	// Now load the profile before trying to retrieve the values of the rest of these.

	LoadProfile( g_ActiveProfile );

	CFG_GET_USER_VAL("xres", Int, g_xres);
	CFG_GET_USER_VAL("yres", Int, g_yres);

	CFG_GET_USER_VAL("vsync", Bool, g_VSync);
	CFG_GET_USER_VAL("novbo", Bool, g_NoGLVBO);
	CFG_GET_USER_VAL("noframebufferobject", Bool, g_NoGLFramebufferObject);
	CFG_GET_USER_VAL("shadows", Bool, g_Shadows);
	CFG_GET_USER_VAL("renderpath", String, g_RenderPath);

	CFG_GET_USER_VAL("lodbias", Float, g_LodBias);

	float gain = -1.0f;
	CFG_GET_USER_VAL("sound.mastergain", Float, gain);
	if(gain > 0.0f)
		WARN_ERR(snd_set_master_gain(gain));

	LOG(NORMAL, LOG_CATEGORY, "g_x/yres is %dx%d", g_xres, g_yres);
	LOG(NORMAL, LOG_CATEGORY, "Active profile is %s", g_ActiveProfile.c_str());
}


static void ParseCommandLineArgs(int argc, char* argv[])
{
	for(int i = 1; i < argc; i++)
	{
		// this arg isn't an option; skip
		if(argv[i][0] != '-')
			continue;

		char* name = argv[i]+1;	// no leading '-'

		// switch first letter of option name
		switch(argv[i][1])
		{
		case 'a':
			if(strncmp(name, "autostart=", 10) == 0)
				 g_AutostartMap = argv[i]+11;
			break;
		case 'b':
			if(!strcmp(name, "buildarchive"))
				// note: VFS init is sure to have been completed by now
				// (since CONFIG_Init reads from file); therefore,
				// it is safe to call this from here directly.
				vfs_opt_rebuild_main_archive("mods/official/official1.zip", "../logs/trace.txt");
			break;
		case 'c':
			if(strcmp(name, "conf") == 0)
			{
				if(argc-i >= 1) // at least one arg left
				{
					i++;
					char* arg = argv[i];
					char* equ = strchr(arg, '=');
					if(equ)
					{
						*equ = 0;
						g_ConfigDB.CreateValue(CFG_COMMAND, arg)
							->m_String = (equ+1);
					}
				}
			}
			break;
		case 'e':
			if(strncmp(name, "entgraph", 8) == 0)
				g_EntGraph = true;
			break;
		case 'f':
			if(strncmp(name, "fixedframe", 10) == 0)
				g_FixedFrameTiming = true;
			break;
		case 'g':
			if(strncmp(name, "g=", 2) == 0)
			{
				g_Gamma = (float)atof(argv[i] + 3);
				if(g_Gamma == 0.0f)
					g_Gamma = 1.0f;
			}
			break;
		case 'l':
			if(strncmp(name, "listfiles", 9) == 0)
				trace_enable(true);
			break;
		case 'n':
			if(strncmp(name, "novbo", 5) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "novbo")->m_String="true";
			break;
		case 'p':
			if(strncmp(name, "profile=", 8) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "profile")->m_String = argv[i]+9;
			break;
		case 'q':
			if(strncmp(name, "quickstart", 10) == 0)
				g_Quickstart = true;
			break;
		case 's':
			if(strncmp(name, "shadows", 7) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "shadows")->m_String="true";
			break;
		case 'v':
			if(strncmp(name, "vsync", 5) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "vsync")->m_String="true";
			break;
		case 'x':
			if(strncmp(name, "xres=", 5) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "xres")->m_String=argv[i]+6;
			break;
		case 'y':
			if(strncmp(name, "yres=", 5) == 0)
				g_ConfigDB.CreateValue(CFG_COMMAND, "yres")->m_String=argv[i]+6;
			break;
		}	// switch
	}
}


void CONFIG_Init(int argc, char* argv[])
{
	debug_printf("CFG_Init &argc=%p &argv=%p\n", &argc, &argv);

	TIMER("CONFIG_Init");
	MICROLOG(L"init config");

	new CConfigDB;

	g_ConfigDB.SetConfigFile(CFG_SYSTEM, false, "config/system.cfg");
	g_ConfigDB.Reload(CFG_SYSTEM);

	g_ConfigDB.SetConfigFile(CFG_MOD, true, "config/mod.cfg");
	// No point in reloading mod.cfg here - we haven't mounted mods yet

	ParseCommandLineArgs(argc, argv);

	// Collect information from system.cfg, the profile file,
	// and any command-line overrides to fill in the globals.
	LoadGlobals();
}
