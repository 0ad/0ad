#include "precompiled.h"

#include "lib/posix/posix_utsname.h"
#include "lib/posix/posix_sock.h"
#include "lib/res/file/path.h"
#include "lib/res/file/vfs.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/sysdep/gfx.h"
#include "lib/sysdep/snd.h"
#include "lib/sysdep/cpu.h"
#include "lib/res/res.h"
#include "lib/res/graphics/tex.h"

#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Game.h"
#include "renderer/Renderer.h"
#include "maths/MathUtil.h"
#include "graphics/GameView.h"

static std::string SplitExts(const char *exts)
{
	std::string str = exts;
	std::string ret = "";
	size_t idx = str.find_first_of(" ");
	while(idx != std::string::npos)
	{
		if(idx >= str.length() - 1)
		{
			ret += str;
			break;
		}

		ret += str.substr(0, idx);
		ret += "\n";
		str = str.substr(idx + 1);
		idx = str.find_first_of(" ");
	}

	return ret;
}


void WriteSystemInfo()
{
	TIMER("write_sys_info");

	// get_cpu_info and gfx_detect already called during init - see call site
	snd_detect();
	get_mem_status();

	struct utsname un;
	uname(&un);

	char N_path[PATH_MAX];
	(void)file_make_full_native_path("../logs/system_info.txt", N_path);
	FILE* f = fopen(N_path, "w");
	if(!f)
		return;

	// .. OS
	fprintf(f, "OS             : %s %s (%s)\n", un.sysname, un.release, un.version);

	// .. CPU
	fprintf(f, "CPU            : %s, %s", un.machine, cpu_type);
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
	fprintf(f, "Memory         : %lu MiB; %lu MiB free\n", tot_mem/MiB, avl_mem/MiB);

	// .. graphics
	fprintf(f, "Graphics Card  : %s\n", gfx_card);
	fprintf(f, "OpenGL Drivers : %s; %s\n", glGetString(GL_VERSION), gfx_drv_ver);
	fprintf(f, "Video Mode     : %dx%d:%d@%d\n", g_xres, g_yres, g_bpp, g_freq);

	// .. sound
	fprintf(f, "Sound Card     : %s\n", snd_card);
	fprintf(f, "Sound Drivers  : %s\n", snd_drv_ver);


	//
	// .. network name / ips
	//

	// note: can't use un.nodename because it is for an
	// "implementation-defined communications network".
	char hostname[128] = "(unknown)";
	(void)gethostname(hostname, sizeof(hostname)-1);
	// -1 makes sure it's 0-terminated. if the function fails,
	// we display "(unknown)" and will skip IP output below.
	fprintf(f, "Network Name   : %s", hostname);

	{
		hostent* host = gethostbyname(hostname);
		if(!host)
			goto no_ip;
		struct in_addr** ips = (struct in_addr**)host->h_addr_list;
		if(!ips)
			goto no_ip;

		// output all IPs (> 1 if using VMware or dual ethernet)
		fprintf(f, " (");
		for(uint i = 0; i < 256 && ips[i]; i++)	// safety
		{
			// separate entries but avoid trailing comma
			if(i != 0)
				fprintf(f, ", ");
			fprintf(f, "%s", inet_ntoa(*ips[i]));
		}
		fprintf(f, ")");
	}

no_ip:
	fprintf(f, "\n");


	// .. OpenGL extensions (write them last, since it's a lot of text)
	const char* exts = oglExtList();
	if (!exts) exts = "{unknown}";
	fprintf(f, "\nOpenGL Extensions: \n%s\n", SplitExts(exts).c_str());

	fclose(f);
	f = 0;
}


// not thread-safe!
static const wchar_t* HardcodedErrorString(int err)
{
	char description[200];
	error_description_r((LibError)err, description, ARRAY_SIZE(description));
	static wchar_t output_buf[200];
	mbstowcs(output_buf, description, ARRAY_SIZE(output_buf));
	return output_buf;
}

// not thread-safe!
const wchar_t* ErrorString(int err)
{
	// language file not available (yet)
	return HardcodedErrorString(err);

	// TODO: load from language file
}


static NextNumberedFilenameInfo screenshot_nfi;

// <extension> identifies the file format that is to be written
// (case-insensitive). examples: "bmp", "png", "jpg".
// BMP is good for quick output at the expense of large files.
void WriteScreenshot(const char* extension)
{
	// get next available numbered filename
	// .. bake extension into format string.
	// note: %04d -> always 4 digits, so sorting by filename works correctly.
	char file_format_string[PATH_MAX];
	snprintf(file_format_string, PATH_MAX, "screenshots/screenshot%%04d.%s", extension);
	file_format_string[PATH_MAX-1] = '\0';
	char fn[PATH_MAX];
	next_numbered_filename(file_format_string, &screenshot_nfi, fn);
	const char* atom_fn = file_make_unique_fn_copy(fn);

	const int w = g_xres, h = g_yres;
	const int bpp = 24;
	GLenum fmt = GL_RGB;
	int flags = TEX_BOTTOM_UP;
	// we want writing BMP to be as fast as possible,
	// so read data from OpenGL in BMP format to obviate conversion.
	if(!stricmp(extension, "bmp"))
	{
		fmt = GL_BGR;
		flags |= TEX_BGR;
	}

	const size_t img_size = w * h * bpp/8;
	const size_t hdr_size = tex_hdr_size(fn);
	FileIOBuf buf = file_buf_alloc(hdr_size+img_size, atom_fn, FB_FROM_HEAP);
	GLvoid* img = (u8*)buf + hdr_size;
	Tex t;
	if(tex_wrap(w, h, bpp, flags, img, &t) < 0)
		return;
	glReadPixels(0, 0, w, h, fmt, GL_UNSIGNED_BYTE, img);
	(void)tex_write(&t, fn);
	(void)tex_free(&t);
	(void)file_buf_free(buf, FB_FROM_HEAP);
}



// Similar to WriteScreenshot, but generates an image of size 640*tiles x 480*tiles.
void WriteBigScreenshot(const char* extension, int tiles)
{
	// get next available numbered filename
	// .. bake extension into format string.
	// note: %04d -> always 4 digits, so sorting by filename works correctly.
	char file_format_string[PATH_MAX];
	snprintf(file_format_string, PATH_MAX, "screenshots/screenshot%%04d.%s", extension);
	file_format_string[PATH_MAX-1] = '\0';
	char fn[PATH_MAX];
	next_numbered_filename(file_format_string, &screenshot_nfi, fn);
	const char* atom_fn = file_make_unique_fn_copy(fn);

	// Slightly ugly and inflexible: Always draw 640*480 tiles onto the screen, and
	// hope the screen is actually large enough for that.
	const int tile_w = 640, tile_h = 480;
	debug_assert(g_xres >= tile_w && g_yres >= tile_h);

	const int img_w = tile_w*tiles, img_h = tile_h*tiles;
	const int bpp = 24;
	GLenum fmt = GL_RGB;
	int flags = TEX_BOTTOM_UP;
	// we want writing BMP to be as fast as possible,
	// so read data from OpenGL in BMP format to obviate conversion.
	if(!stricmp(extension, "bmp"))
	{
		fmt = GL_BGR;
		flags |= TEX_BGR;
	}

	const size_t img_size = img_w * img_h * bpp/8;
	const size_t tile_size = tile_w * tile_h * bpp/8;
	const size_t hdr_size = tex_hdr_size(fn);
	void* tile_data = malloc(tile_size);
	if(!tile_data)
		WARN_ERR_RETURN(ERR::NO_MEM);
	FileIOBuf img_buf = file_buf_alloc(hdr_size+img_size, atom_fn, FB_FROM_HEAP);

	Tex t;
	GLvoid* img = (u8*)img_buf + hdr_size;
	if(tex_wrap(img_w, img_h, bpp, flags, img, &t) < 0)
		return;

	oglCheck();

	// Resize various things so that the sizes and aspect ratios are correct
	{
		g_Renderer.Resize(tile_w, tile_h);
		SViewPort vp = { 0, 0, tile_w, tile_h };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);
	}

	// Temporarily move everything onto the front buffer, so the user can
	// see the exciting progress as it renders (and can tell when it's finished).
	// (It doesn't just use SwapBuffers, because it doesn't know whether to
	// call the SDL version or the Atlas version.)
	GLint oldReadBuffer, oldDrawBuffer;
	glGetIntegerv(GL_READ_BUFFER, &oldReadBuffer);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuffer);
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);

	// Render each tile
	for (int tile_y = 0; tile_y < tiles; ++tile_y)
	{
		for (int tile_x = 0; tile_x < tiles; ++tile_x)
		{
			// Adjust the camera to render the appropriate region
			g_Game->GetView()->GetCamera()->SetProjectionTile(tiles, tile_x, tile_y);

			Render();

			// Copy the tile pixels into the main image
			glReadPixels(0, 0, tile_w, tile_h, fmt, GL_UNSIGNED_BYTE, tile_data);
			for (int y = 0; y < tile_h; ++y)
			{
				void* dest = (char*)img + ((tile_y*tile_h + y) * img_w + (tile_x*tile_w)) * bpp/8;
				void* src = (char*)tile_data + y * tile_w * bpp/8;
				memcpy2(dest, src, tile_w * bpp/8);
			}
		}
	}

	// Restore the buffer settings
	glDrawBuffer(oldDrawBuffer);
	glReadBuffer(oldReadBuffer);

	// Restore the viewport settings
	{
		g_Renderer.Resize(g_xres, g_yres);
		SViewPort vp = { 0, 0, g_xres, g_yres };
		g_Game->GetView()->GetCamera()->SetViewPort(&vp);
		g_Game->GetView()->GetCamera()->SetProjection(CGameView::defaultNear, CGameView::defaultFar, CGameView::defaultFOV);

		g_Game->GetView()->GetCamera()->SetProjectionTile(1, 0, 0);
	}

	(void)tex_write(&t, fn);
	(void)tex_free(&t);
	free(tile_data);
	(void)file_buf_free(img_buf, FB_FROM_HEAP);
}
