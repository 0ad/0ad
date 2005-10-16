#include "precompiled.h"

#include "lib/res/file/vfs.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/detect.h"
#include "lib/res/res.h"
#include "lib/res/graphics/tex.h"

#include "ps/GameSetup/Config.h"

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
	TIMER(write_sys_info);

	// get_cpu_info and get_gfx_info already called during init - see call site
	get_snd_info();
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



static const wchar_t* HardcodedErrorString(int err)
{
#define E(sym) case sym: return L ## #sym;

	switch(err)
	{
	E(ERR_NO_MEM)
	E(ERR_FILE_NOT_FOUND)
	E(ERR_INVALID_HANDLE)
	E(ERR_INVALID_PARAM)
	E(ERR_EOF)
	E(ERR_PATH_NOT_FOUND)
	E(ERR_PATH_LENGTH)
	default:
		return 0;
	}
}

const wchar_t* ErrorString(int err)
{
	// language file not available (yet)
	return HardcodedErrorString(err);

	// TODO: load from language file
}


// <extension> identifies the file format that is to be written
// (case-insensitive). examples: "bmp", "png", "jpg".
// BMP is good for quick output at the expense of large files.
void WriteScreenshot(const char* extension)
{
	// determine next screenshot number.
	//
	// current approach: increment number until that file doesn't yet exist.
	// this is fairly slow, but it's typically only done once, since the last
	// number is cached. binary search shouldn't be necessary.
	//
	// known bug: after program restart, holes in the number series are
	// filled first. example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> it gets number 1, not 3. 
	// could fix via enumerating all files, but it's not worth it ATM.
	char fn[VFS_MAX_PATH];

	// %04d -> always 4 digits, so sorting by filename works correctly.
	const char* file_format_string = "screenshots/screenshot%04d.%s";

	static int next_num = 1;
	do
		sprintf(fn, file_format_string, next_num++, extension);
	while(vfs_exists(fn));

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
	Handle img_hm;
	void* data = mem_alloc(hdr_size+img_size, FILE_BLOCK_SIZE, 0, &img_hm);
	if(!data)
	{
		debug_warn("not enough memory to write screenshot");
		return;
	}
	GLvoid* img = (u8*)data + hdr_size;
	Tex t;
	if(tex_wrap(w, h, bpp, flags, img, &t) < 0)
		return;
	glReadPixels(0, 0, w, h, fmt, GL_UNSIGNED_BYTE, img);
	(void)tex_write(&t, fn);
	(void)tex_free(&t);
	mem_free_h(img_hm);
}
