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

u32 game_ticks;

int xres = 800, yres = 600;

u32 font;
u32 tex;


// error in SDL before OpenGL initialized;
// display error message, and quit
// TODO: localization

enum SDLError
{
	INIT,
	VMODE
};

#ifdef _WIN32
extern "C" {
#define MB_ICONEXCLAMATION 0x30
__declspec(dllimport) unsigned long __stdcall MessageBoxA(void*, const char*, const char*, unsigned int);
}
#endif

static void sdl_error(SDLError err)
{
	char msg[1000] = "Problem while setting up OpenGL.\n"\
	                 "Details: ";
	char* pos = msg + strlen(msg);
	int rem = sizeof(msg)-1 - (pos-msg);	// space remaining in buffer

	if(err == INIT)
		snprintf(pos, rem, "SDL library initialization failed (%s)\n", SDL_GetError());
	else if(err == VMODE)
		snprintf(pos, rem, "could not set %dx%d graphics mode (%s)\n", xres, yres, SDL_GetError());

	fprintf(stderr, msg);

#ifdef _WIN32
	MessageBoxA(0, msg, "0ad", MB_ICONEXCLAMATION);
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


static bool keys[256];

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
			exit(0);
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
	glClear(GL_COLOR_BUFFER_BIT);

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
	glOrtho(0., xres, 0., yres, -1., 1.);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// logo
	glLoadIdentity();
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

	// FPS counter
	glLoadIdentity();
	glTranslatef(10, 30, 0);
	font_bind(font);
	glprintf("%d FPS", fps);

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


void main(int argc, char* argv[])
{
	// set 24 bit (float) FPU precision for faster divides / sqrts
#ifdef _M_IX86
	_control87(_PC_24, _MCW_PC);
#endif

	detect();

	// output system information
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
	printf("%s\n", un.nodename);
	char hostname[100];	// possibly nodename != hostname
	gethostname(hostname, sizeof(hostname));
	hostent* h = gethostbyname(hostname);
	if(h)
	{
		struct in_addr** ips = (struct in_addr**)h->h_addr_list;
		for(int i = 0; ips && ips[i]; i++)
			printf("%s ", inet_ntoa(*ips[i]));
		printf("\n");
	}
	fflush(stdout);

	// init SDL
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) < 0)
		sdl_error(INIT);
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
		sdl_error(VMODE);

	// call again - needs OpenGL to be initialized on non-Win32 systems
#ifndef _WIN32
	get_gfx_card();
#endif

	glEnable(GL_TEXTURE_2D);

	vfs_add_search_path("data");
	tex = tex_load("0adlogo1.bmp");
	tex_upload(tex);
	font = font_load("verdana.fnt");


in_add_handler(handler);


// fixed timestep main loop
	const double TICK_TIME = 30e-3;	// [s]
	double time0 = get_time();
	for(;;)
	{
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
}
