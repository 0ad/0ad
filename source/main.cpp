#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "wsdl.h"
#include "ogl.h"
#include "detect.h"
#include "time.h"
#include "input.h"
#include "misc.h"
#include "posix.h"
#include "font.h"
#include "res.h"
#include "tex.h"
#include "vfs.h"
#include "ia32.h"
#include "Config.h"

#ifndef NO_GUI
#include "gui/GUI.h"
#endif


u32 game_ticks;

int xres = 800, yres = 600;

bool keys[256];

#include <cmath>

static Handle font;
static Handle tex;

extern void terr_init();
extern void terr_update();
extern bool terr_handler(const SDL_Event& ev);


static void write_sys_info()
{
	get_gfx_card();

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


#ifdef _WIN32
#define MB_ICONEXCLAMATION 0x30
IMP(int, MessageBoxA, (void*, const char*, const char*, unsigned int))
#endif

// error before GUI is initialized: display message, and quit
// TODO: localization
static void display_startup_error(const char* msg)
{
	const char* caption = "0ad startup problem";

	fprintf(stderr, msg);

	write_sys_info();

#ifdef _WIN32
	MessageBoxA(0, msg, caption, MB_ICONEXCLAMATION);
#endif

	exit(1);
}


static int set_vmode(int w, int h, int bpp)
{
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if(!SDL_SetVideoMode(w, h, bpp, SDL_OPENGL|SDL_FULLSCREEN))
		return -1;

	glViewport(0, 0, w, h);

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
#ifndef NO_GUI
	////// janwas: I changed to some more for the GUI, we can talk about how to set this up
	glOrtho(0., xres, 0., yres, -1000., 1.);
	//////
#else
	// (MT) Above line hides the frame counter behind the terrain.
	glOrtho( 0., xres, 0., yres, -1., 1. );
#endif
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
	// set 24 bit (float) FPU precision for faster divides / sqrts
#ifdef _M_IX86
	_control87(_PC_24, _MCW_PC);
#endif

	detect();

	// init SDL
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0)
	{
		char buf[1000];
		snprintf(buf, sizeof(buf), "SDL library initialization failed: %s\n", SDL_GetError());
		display_startup_error(buf);
	}
	atexit(SDL_Quit);

	// preferred video mode = current desktop settings
	// (command line params may override these)
	get_cur_resolution(xres, yres);

	for(int a = 1; a < argc; a++)
		if(!strncmp(argv[a], "xres", 4))
			xres = atoi(argv[a]+4);
		else if(!strncmp(argv[a], "yres", 4))
			yres = atoi(argv[a]+4);
		// TODO: other command line options

	if(set_vmode(xres, yres, 32) < 0)
	{
		char buf[1000];
		snprintf(buf, sizeof(buf), "could not set %dx%d graphics mode: %s\n", xres, yres, SDL_GetError());
		display_startup_error(buf);
	}

	write_sys_info();

if(!oglExtAvail("GL_ARB_multitexture") || !oglExtAvail("GL_ARB_texture_env_combine"))
	exit(1);
glEnable (GL_CULL_FACE);
glEnable (GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);

	new CConfig;

// TODO: all loading should go through the VFS; for now, we allow normal access
// this has to come before VFS init, as it changes the current dir
#ifndef NO_GUI
	new CGUI; // we should have a place for all singleton news
	g_GUI.Initialize();
	g_GUI.LoadXMLFile("hello.xml");
	//g_GUI.LoadXMLFile("sprite1.xml");
#endif

	vfs_set_root(argv[0], "data");

//	tex = tex_load("0adlogo2.bmp");
//	tex_upload(tex);
	font = font_load("verdana.fnt");





terr_init();

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
