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

#ifndef NO_GUI
#include "gui/GUI.h"
#endif


u32 game_ticks;

bool keys[256];

#include <cmath>

static Handle font;
static Handle tex;

extern void terr_init();
extern void terr_update();
extern bool terr_handler(const SDL_Event& ev);


static void write_sys_info()
{
	get_gfx_info();

	struct utsname un;
	uname(&un);
	freopen("stdout.txt", "w", stdout);
	// .. OS
	printf("%s %s (%s)\n", un.sysname, un.release, un.version);
	// .. CPU
	printf("%s, %s", un.machine, cpu_type);
	if(cpu_freq != 0.0f)
	{
		if(cpu_freq < 1e9)
			printf(", %.2f MHz\n", cpu_freq*1e-6);
		else
			printf(", %.2f GHz\n", cpu_freq*1e-9);
	}
	else
		printf("\n");
	// .. memory
	printf("%lu MB RAM; %lu MB free\n", tot_mem/MB, avl_mem/MB);
	// .. graphics card
	printf("%s\n", gfx_card);
	printf("%s\n", gfx_drv);
	// .. network name / ips
	char hostname[100];	// possibly nodename != hostname
	gethostname(hostname, sizeof(hostname));
	printf("%s\n", hostname);
	hostent* h = gethostbyname(hostname);
	if(h)
	{
		struct in_addr** ips = (struct in_addr**)h->h_addr_list;
		for(int i = 0; ips && ips[i]; i++)
			printf("%s ", inet_ntoa(*ips[i]));
		printf("\n");
	}
	fflush(stdout);
}


// error before GUI is initialized: display message, and quit
// TODO: localization
static void display_startup_error(const wchar_t* msg)
{
	const wchar_t* caption = L"0ad startup problem";

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
	}

	return 0;
}


static void render()
{
// TODO: not needed with 100% draw coverage
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	terr_update();

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

	// logo
/*	glLoadIdentity();
	glTranslatef(50, 100, 0);
	tex_bind(tex);
	glBegin(GL_QUADS);

const float u = 0.585f, v = 1.0f;
const float x = 600.0f, y = 512.0f;
	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(u, 0); glVertex2f(x, 0);
	glTexCoord2f(u, v); glVertex2f(x, y);
	glTexCoord2f(0, v); glVertex2f(0, y);
	glEnd();
*/
/*	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(111, 0);
		glVertex2i(111, 111);
		glVertex2i(0, 111);
	glEnd();
*/
	// FPS counter
	glLoadIdentity();
	glTranslatef(10, 30, 0);
	font_bind(font);
	glprintf("%d FPS", fps);

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
}


static void do_tick()
{
}

int main(int argc, char* argv[])
{
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

	const int ERR_MSG_SIZE = 1000;
	wchar_t err_msg[ERR_MSG_SIZE];

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
glEnable (GL_CULL_FACE);
glEnable (GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);

	new CConfig;

	vfs_set_root(argv[0], "data");

#ifndef NO_GUI
	// GUI uses VFS, so this must come after VFS init.
	g_GUI.Initialize();
	g_GUI.LoadXMLFile("gui/styles.xml");
	g_GUI.LoadXMLFile("gui/hello.xml");
	g_GUI.LoadXMLFile("gui/sprite1.xml");
#endif

//	tex = tex_load("0adlogo2.bmp");
//	tex_upload(tex);
	font = font_load("verdana.fnt");





terr_init();


#ifndef NO_GUI
	in_add_handler(gui_handler);
#endif
in_add_handler(handler);
in_add_handler(terr_handler);


// fixed timestep main loop
	const double TICK_TIME = 30e-3;	// [s]
	double time0 = get_time();

	g_Config.Update();
	while(!quit)
	{
		g_Config.Update();

// TODO: limiter in case simulation can't keep up?
		double time1 = get_time();
		while((time1-time0) > TICK_TIME)
		{
			game_ticks++;
			in_get_events();
			do_tick();
			time0 += TICK_TIME;
		}

		render();
		SDL_GL_SwapBuffers();

		calc_fps();

	}

#ifndef NO_GUI
	g_GUI.Destroy();
	delete CGUI::GetSingletonPtr(); // again, we should have all singleton deletes somewhere
#endif

	delete &g_Config;

	return 0;
}
