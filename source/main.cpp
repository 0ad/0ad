#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "sdl.h"
#include "ogl.h"
#include "detect.h"
#include "timer.h"
#include "input.h"
#include "lib.h"
#include "posix.h"
#include "res/res.h"
#ifdef _M_IX86
#include "sysdep/ia32.h"
#endif
#include "ps/Config.h"
#include "MapReader.h"
#include "Terrain.h"
#include "Renderer.h"
#include "Model.h"
#include "UnitManager.h"

#include "BaseEntityCollection.h"
#include "Entity.h"
#include "EntityHandles.h"
#include "EntityManager.h"

#ifndef NO_GUI
#include "gui/GUI.h"
#endif



u32 game_ticks;

bool keys[SDLK_LAST];
bool mouseButtons[5];

#include <cmath>

// flag to disable extended GL extensions until fix found - specifically, crashes
// using VBOs on laptop Radeon cards
static bool g_NoGLVBO=false;

static bool g_VSync = false;

static float g_Gamma = 1.0f;

// mapfile to load or null for no map (and to use default terrain)
static const char* g_MapFile=0;


static Handle font;


extern CCamera g_Camera;

extern void terr_init();
extern void terr_update(float time);
extern bool terr_handler(const SDL_Event& ev);

extern int allow_reload();
extern int dir_add_watch(const char* const dir, bool watch_subdirs);



static int write_sys_info()
{
	get_gfx_info();

	struct utsname un;
	uname(&un);

	FILE* const f = fopen("../system/system_info.txt", "w");
	if(!f)
		return -1;

	// .. OS
	fprintf(f, "%s %s (%s)\n", un.sysname, un.release, un.version);
	// .. CPU
	fprintf(f, "%s, %s", un.machine, cpu_type);
	if(cpus > 1)
		fprintf(f, " (x%d)", cpus);
	if(cpu_freq != 0.0f)
	{
		if(cpu_freq < 1e9)
			fprintf(f, ", %.2f MHz\n", cpu_freq*1e-6);
		else
			fprintf(f, ", %.2f GHz\n", cpu_freq*1e-9);
	}
	else
		fprintf(f, "\n");
	// .. memory
	fprintf(f, "%lu MB RAM; %lu MB free\n", tot_mem/MB, avl_mem/MB);
	// .. graphics card
	fprintf(f, "%s\n", gfx_card);
	fprintf(f, "%s\n", gfx_drv);
	// .. network name / ips
	char hostname[100];	// possibly nodename != hostname
	gethostname(hostname, sizeof(hostname));
	fprintf(f, "%s\n", hostname);
	hostent* h = gethostbyname(hostname);
	if(h)
	{
		struct in_addr** ips = (struct in_addr**)h->h_addr_list;
		for(int i = 0; ips && ips[i]; i++)
			fprintf(f, "%s ", inet_ntoa(*ips[i]));
		fprintf(f, "\n");
	}
	
	fclose(f);
	return 0;
}


// error before GUI is initialized: display message, and quit
// TODO: localization
static void display_startup_error(const wchar_t* msg)
{
	const wchar_t* caption = L"0ad startup problem";

	write_sys_info();
	wdisplay_msg(caption, msg);
	exit(1);
}

// error before GUI is initialized: display message, and quit
// TODO: localization
static void display_startup_error(const char* msg)
{
	const char* caption = "0ad startup problem";

	write_sys_info();
	display_msg(caption, msg);
	exit(1);
}


static int set_vmode(int w, int h, int bpp)
{
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(!SDL_SetVideoMode(w, h, bpp, SDL_OPENGL|SDL_FULLSCREEN))
		return -1;

	glViewport(0, 0, w, h);

#ifndef NO_GUI
	g_GUI.UpdateResolution();
#endif

	oglInit();	// required after each mode change

	return 0;
}


// break out of main loop
static bool quit = false;

static bool handler(const SDL_Event& ev)
{
	int c;

	switch(ev.type)
	{
	case SDL_KEYDOWN:
		c = ev.key.keysym.sym;
		keys[c] = true;
		switch(c)
		{
		case SDLK_ESCAPE:
			quit = true;
			break;
		}
		break;

	case SDL_KEYUP:
		c = ev.key.keysym.sym;
		keys[c] = false;
		break;
	case SDL_MOUSEBUTTONDOWN:
		c = ev.button.button;
		if( c < 5 )
			mouseButtons[c] = true;
		break;
	case SDL_MOUSEBUTTONUP:
		c = ev.button.button;
		if( c < 5 )
			mouseButtons[c] = false;
		break;
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// RenderTerrain: iterate through all terrain patches and submit all patches in viewing frustum to
// the renderer
void RenderTerrain()
{
	CFrustum frustum=g_Camera.GetFustum();
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (uint j=0; j<patchesPerSide; j++) {
		for (uint i=0; i<patchesPerSide; i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			if (frustum.IsBoxVisible (CVector3D(0,0,0),patch->GetBounds())) {
				g_Renderer.Submit(patch);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// RenderModels: iterate through model list and submit all models in viewing frustum to the 
// Renderer
void RenderModels()
{
	CFrustum frustum=g_Camera.GetFustum();

	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	uint i;
	for (i=0;i<units.size();++i) {
		if (frustum.IsBoxVisible(CVector3D(0,0,0),units[i]->m_Model->GetBounds())) {
			g_Renderer.Submit(units[i]->m_Model);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// RenderNoCull: render absolutely everything to a blank frame to force renderer
// to load required assets 
void RenderNoCull()
{
	g_Renderer.BeginFrame();
	g_Renderer.SetCamera(g_Camera);

	uint i,j;
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (i=0;i<units.size();++i) {
		g_Renderer.Submit(units[i]->m_Model);
	}
	
	u32 patchesPerSide=g_Terrain.GetPatchesPerSide();
	for (j=0; j<patchesPerSide; j++) {
		for (i=0; i<patchesPerSide; i++) {
			CPatch* patch=g_Terrain.GetPatch(i,j);
			g_Renderer.Submit(patch);
		}
	}

	g_Renderer.FlushFrame();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	g_Renderer.EndFrame();
}


static void Render()
{
	// start new frame
	g_Renderer.BeginFrame();
	g_Renderer.SetCamera(g_Camera);

	// switch on wireframe for terrain if we want it
	g_Renderer.SetTerrainRenderMode( SOLID );

	RenderTerrain();
	RenderModels();
	g_Renderer.FlushFrame();

	glColor3f(1.0f, 1.0f, 1.0f);

	// overlay mode
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glAlphaFunc(GL_GREATER, 0.5);
	glEnable(GL_ALPHA_TEST);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho(0.f, (float)g_xres, 0.f, (float)g_yres, -1.f, 1000.f);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// FPS counter
	glLoadIdentity();
	glTranslatef(10, 30, 0);
	font_bind(font);
	glprintf("%d FPS", fps);

	// view params
	glLoadIdentity();
	glTranslatef(10, 90, 0);
	extern float ViewFOV;
	glprintf("FOV=%.1f", RADTODEG(ViewFOV));

#ifndef NO_GUI
	// Temp GUI message GeeTODO
	glLoadIdentity();
	glTranslatef(10, 60, 0);
	glprintf("%s", g_GUI.TEMPmessage.c_str());

	glLoadIdentity();
	g_GUI.Draw();
#endif

	// restore
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	g_Renderer.EndFrame();
}


static void do_tick()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////
// UpdateWorld: update time dependent data in the world to account for changes over 
// the given time (in s)
void UpdateWorld(float time)
{
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		units[i]->m_Model->Update(time);
	}

	g_EntityManager.updateAll( time );

}


void ParseArgs(int argc, char* argv[])
{
	for (int i=1;i<argc;i++) {
		if (argv[i][0]=='-') {
			switch (argv[i][1]) {
				case 'm':
					if (argv[i][2]=='=') {
						g_MapFile=argv[i]+3;
					}
					break;
				case 'g':
					if( argv[i][2] == '=' )
					{
						g_Gamma = (float)atof( argv[i] + 3 );
						if( g_Gamma == 0.0f ) g_Gamma = 1.0f;
					}
					break;
				case 'v':
					g_VSync = true;
					break;
				case 'n':
					if (strncmp(argv[i]+1,"novbo",5)==0) {
						g_NoGLVBO=true;
					}
					break;
			}
		}
	}
}



int main(int argc, char* argv[])
{
	const int ERR_MSG_SIZE = 1000;
	wchar_t err_msg[ERR_MSG_SIZE];

//	display_startup_error(argv[0]);


	// set current directory to "$game_dir/data".
	// this is necessary because it is otherwise unknown,
	// especially if run from a shortcut, batch file, or symlink.
	//
	// "../data" is relative to the executable (in "$game_dir/system").
	//
	// rationale for data/ being root: untrusted scripts must not be
	// allowed to overwrite critical game (or worse, OS) files.
	// the VFS prevents any accesses to files above this directory.
	int err = file_rel_chdir(argv[0], "../data");
	if(err < 0)
	{
		swprintf(err_msg, ERR_MSG_SIZE, L"error setting current directory.\n"\
			L"argv[0] is probably incorrect. please start the game via command-line.");
		display_startup_error(err_msg);
	}


// default to loading map for convenience during development
// override by passing any parameter.
if(argc < 2)
{
	argc = 2;
	argv[1] = "-m=test01.pmp";
}

	ParseArgs(argc, argv);


	lib_init();

	// set 24 bit (float) FPU precision for faster divides / sqrts
#ifdef _M_IX86
	_control87(_PC_24, _MCW_PC);
#endif

	detect();

	// GUI is notified in set_vmode, so this must come before that.
#ifndef NO_GUI
	new CGUI;
#endif


	// init SDL
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0)
	{
		swprintf(err_msg, ERR_MSG_SIZE, L"SDL library initialization failed: %s\n", SDL_GetError());
		display_startup_error(err_msg);
	}
	atexit(SDL_Quit);
	SDL_EnableUNICODE(1);

	// preferred video mode = current desktop settings
	// (command line params may override these)
	get_cur_resolution(g_xres, g_yres);

	for(int a = 1; a < argc; a++)
		if(!strncmp(argv[a], "xres", 4))
			g_xres = atoi(argv[a]+4);
		else if(!strncmp(argv[a], "yres", 4))
			g_yres = atoi(argv[a]+4);
		// TODO: other command line options

	if(set_vmode(g_xres, g_yres, 32) < 0)
	{
		swprintf(err_msg, ERR_MSG_SIZE, L"could not set %dx%d graphics mode: %s\n", g_xres, g_yres, SDL_GetError());
		display_startup_error(err_msg);
	}

	write_sys_info();

	if(!oglExtAvail("GL_ARB_multitexture") || !oglExtAvail("GL_ARB_texture_env_combine"))
		display_startup_error(L"required ARB_multitexture or ARB_texture_env_combine extension not available");

	// enable/disable VSync
	// note: "GL_EXT_SWAP_CONTROL" is "historical" according to dox.
	if(oglExtAvail("WGL_EXT_swap_control"))
		wglSwapIntervalEXT(g_VSync? 1 : 0);


	if(SDL_SetGamma(g_Gamma, g_Gamma, g_Gamma) < 0)
	{
		debug_warn("SDL_SetGamma failed");
	}

	new CConfig;

	vfs_mount("", "mods/official/", 0);
////	dir_add_watch("mods\\official", false);

#ifndef NO_GUI
	// GUI uses VFS, so this must come after VFS init.
	g_GUI.Initialize();
	g_GUI.LoadXMLFile("gui/styles.xml");
	g_GUI.LoadXMLFile("gui/hello.xml");
	g_GUI.LoadXMLFile("gui/sprite1.xml");
#endif


	font = font_load("verdana.fnt");

	// set renderer options from command line options - NOVBO must be set before opening the renderer
	g_Renderer.SetOption(CRenderer::OPT_NOVBO,g_NoGLVBO);

	// terr_init actually opens the renderer and loads a bunch of resources as well as setting up
	// the terrain
	terr_init();


	// This needs to be done after the renderer has loaded all its actors...
	new CBaseEntityCollection;
	new CEntityManager;

	g_EntityTemplateCollection.loadTemplates();


	// load a map if we were given one
	if (g_MapFile) {
		CStr mapfilename("mods/official/maps/scenarios/");
		mapfilename+=g_MapFile;
		try {
			CMapReader reader;
			reader.LoadMap(mapfilename);
		} catch (...) {
			char errmsg[256];
			sprintf(errmsg, "Failed to load map %s\n", mapfilename.c_str());
			display_startup_error(errmsg);		
		}
	}
	
	// Initialize entities

	g_EntityManager.dispatchAll( &CMessage( CMessage::EMSG_INIT ) );

#ifndef NO_GUI
	in_add_handler(gui_handler);
#endif
in_add_handler(handler);
in_add_handler(terr_handler);

	// render everything to a blank frame to force renderer to load everything
	RenderNoCull();


// fixed timestep main loop
	const double TICK_TIME = 30e-3;	// [s]
	double time0 = get_time();

	g_Config.Update();
	while(!quit)
	{
		//g_Config.Update();

////		allow_reload();


// TODO: limiter in case simulation can't keep up?
#if 0
		double time1 = get_time();
		while((time1-time0) > TICK_TIME)
		{
			game_ticks++;
			in_get_events();
			do_tick();
			time0 += TICK_TIME;
		}
		UpdateWorld(float(time1-time0));
		terr_update(float(time1-time0));
		Render();
		SDL_GL_SwapBuffers();

		calc_fps();

#else

		double time1 = get_time();

		mouseButtons[SDL_BUTTON_WHEELUP] = false;
		mouseButtons[SDL_BUTTON_WHEELDOWN] = false;

		in_get_events();
		UpdateWorld(float(time1-time0));
		terr_update(float(time1-time0));
		Render();
		SDL_GL_SwapBuffers();

		calc_fps();
		time0=time1;
#endif
	}

	// TODO MT: Move this to atexit() code? Capture original gamma ramp at initialization and restore it?

	SDL_SetGamma( 1.0f, 1.0f, 1.0f );

#ifndef NO_GUI
	g_GUI.Destroy();
	delete CGUI::GetSingletonPtr(); // again, we should have all singleton deletes somewhere
#endif

	delete &g_Config;
	delete &g_EntityManager;
	delete &g_EntityTemplateCollection;

	return 0;
}
