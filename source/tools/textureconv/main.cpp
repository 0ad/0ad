#include "IL/il.h"
#include "IL/ilu.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <assert.h>

#include <string>
#include <vector>
#include <stack>

#include <tchar.h>
#include <time.h>

#ifndef NDEBUG
#include <crtdbg.h>
#endif

#ifdef UNICODE
#define tstring wstring
#define tstrcmp wcscmp
#define tstrrchr wcsrchr
#define tsprintf swprintf
#define tmain wmain
#else
#define tstring string
#define tstrcmp strcmp
#define tstrrchr strrchr
#define tsprintf sprintf
#define tmain main
#endif

const TCHAR* msgbox_title = _T("Wildfire Games - Texture Converter");

enum OutputFileFormat { DXTn, DXT1, DXT3, DXT5, BMP, TGA, BEST };
enum trool { tr_false, tr_true, tr_maybe };
struct ConversionSettings
{
	OutputFileFormat fmt;
	bool mipmaps;
	trool alphablock;
};



void process_args(int argc, TCHAR** argv);

void convert(std::tstring filename, ConversionSettings& settings);


void msg(const TCHAR* message, int icon)
{
	MessageBox(NULL, message, msgbox_title, MB_OK | icon);
}

void die(const TCHAR* message)
{
	msg(message, MB_ICONERROR);
	exit(1);
}

int tmain(int argc, TCHAR** argv)
{
#ifndef NDEBUG
_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
//_CrtSetBreakAlloc(108);
#endif

	if (argc <= 1)
	{
		msg(_T("To run the texture converter, drag-and-drop BMP files onto the Texture Converter's icon."), MB_ICONINFORMATION);
		exit(0);
	}

	ilInit();

	clock_t t=clock();
	process_args(argc-1, argv+1);
	t=clock()-t;

	TCHAR buf[256];
	tsprintf(buf, _T("Conversion complete (%.2f seconds)"), (double)t/CLOCKS_PER_SEC);
	msg(buf, MB_ICONINFORMATION);

	return 0;
}

void check()
{
	ILenum err = ilGetError();
	if (err != IL_NO_ERROR)
	{
		TCHAR buf[256];
		tsprintf(buf, _T("DevIL error '%04x' encountered"), err);
		die(buf);
	}
}


void process_args(int argc, TCHAR** argv)
{
	// Process arguments: Things like "-dxt5" alter the current output format
	// and settings. Brackets allow scoped changes. Anything else is a filename
	// to be converted.
	//
	// Example: "textureconv.exe a.bmp ( -dxt1 b.bmp -dxt3 c.bmp ) d.bmp"
	// will use default settings for a.bmp and d.bmp, DXT1 for b.bmp, and
	// DXT3 for c.bmp.

	std::stack<ConversionSettings> settings;

	ConversionSettings def = { BEST, false, tr_false };
	settings.push(def);

	for (int i = 0; i < argc; ++i)
	{
#define CASE(s) else if (tstrcmp(argv[i], _T(s)) == 0)
		if(0);

		CASE("(")
			settings.push(settings.top());
		CASE(")")
		{
			if (settings.size() <= 1)
				die(_T("Incorrect command-line parenthesis nesting"));
			settings.pop();
		}
		CASE("-dxt1")
			settings.top().fmt = DXT1;
		CASE("-dxt3")
			settings.top().fmt = DXT3;
		CASE("-dxt5")
			settings.top().fmt = DXT5;
		CASE("-bmp")
			settings.top().fmt = BMP;
		CASE("-tga")
			settings.top().fmt = TGA;
		CASE("-mipmaps")
			settings.top().mipmaps = true;
		CASE("-nomipmaps")
			settings.top().mipmaps = false;
		CASE("-alphablock")
			settings.top().alphablock = tr_true;
		CASE("-noalphablock")
			settings.top().alphablock = tr_false;
		else
		{
			ConversionSettings s = settings.top(); // copy
			if (s.fmt == BEST)
			{
				// Convert .dds->BMP, and anything else to DDS
				const TCHAR* dot = tstrrchr(argv[i], _T('.'));
				if (dot && tstrcmp(dot, _T(".dds"))==0)
					s.fmt = BMP;
				else
					s.fmt = DXTn;
			}
			convert(argv[i], s);
		}
#undef CASE
	}
}


void convert(std::tstring filename, ConversionSettings& settings)
{
	// Generate the output .dds/etc filename:
	size_t dot = filename.rfind(_T("."));
	if (dot == filename.npos)
	{
		std::tstring msg = _T("Attempted conversion of invalid filename '") + filename + _T("' - aborting.");
		die(msg.c_str());
	}

//OutputDebugString(filename.c_str());
//OutputDebugString(L"\n");

	const TCHAR* extn;
	switch (settings.fmt)
	{
	case DXTn:
	case DXT1:
	case DXT3:
	case DXT5:
		extn = _T(".dds");
		break;
	case BMP:
		extn = _T(".bmp");
		break;
	case TGA:
		extn = _T(".tga");
		break;
	default:
		die(_T("Internal error - invalid output format"));
	}

	std::tstring filename_out = filename.substr(0, dot) + extn;

	// Load the original image:

	ILuint img;
	ilGenImages(1, &img);
	ilBindImage(img);

	check();

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	check();

	if (! ilLoadImage((TCHAR*) filename.c_str()))
	{
		std::tstring msg = _T("Error loading file '") + filename + _T("' - aborting.");
		die(msg.c_str());
	}

	// Make sure it's in a nice format for future processing
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

	check();

	ILinfo info;
	iluGetImageInfo(&info);

	check();


	// Check for the existence of a non-solid alpha channel (so that it won't
	// be stored when it has no useful data)

	bool has_alpha = false;

	{
		if (info.Type != IL_UNSIGNED_BYTE || info.Format != IL_RGBA)
			die(_T("Internal error - invalid data format"));

		ILubyte* pos = ilGetData();
		ILubyte* end = pos + info.Height*info.Width*info.Bpp;

		for (pos += 3; pos < end; pos += info.Bpp)
		{
			if (*pos != 0xff)
			{
				has_alpha = true;
				break;
			}
		}
	}


	if (settings.fmt == DXTn || settings.fmt == DXT1 || settings.fmt == DXT3 || settings.fmt == DXT5)
	{

		if (settings.alphablock == tr_true || (settings.alphablock == tr_maybe && info.Height == info.Width*2))
		{
			// Reading from file with special alpha mode: colour stored in top
			// half, alpha in bottom half (as greyscale)

			// Start/end of top half
			ILubyte* start = ilGetData();
			ILubyte* end = start + ((info.Width*info.Height)/2) * info.Bpp;
			
			// Get the start/end of the colour and alpha (as greyscale) sections
			ILubyte* buf_c = start + 3; // 4th byte of RGBA
			ILubyte* buf_a = end;
			ILubyte* end_a = end + (end-start);

			// Copy greyscale's R component into colour's A
			for (; buf_a < end_a; buf_c+=4, buf_a+=4)
				*buf_c = *buf_a;

			// Chop off the bottom of the image
			iluCrop(0, 0, 0, info.Width, info.Height/2, 1);

			has_alpha = true;
		}
	}
	else if (settings.fmt == BMP)
	{
		if (has_alpha)
		{
			// Writing to file with special alpha mode: colour stored in top
			// half, alpha in bottom half (as greyscale)

			// Add some space for the new image
			iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);
			iluEnlargeCanvas(info.Width, info.Height*2, 1);

			// Start/end of top half
			ILubyte* start = ilGetData();
			ILubyte* end = start + (info.Width*info.Height) * info.Bpp;

			// Get the start/end of the colour and alpha (as greyscale) sections
			ILubyte* buf_c = start + 3;
			ILubyte* buf_a = end;
			ILubyte* end_a = end + (end-start);

			// Copy colour's A component into greyscale's RGB
			for (; buf_a < end_a; buf_c+=4, buf_a+=4)
			{
				*buf_a = *(buf_a+1) = *(buf_a+2) = *buf_c;
				*buf_c = *(buf_a+3) = 0xff; // clear the alpha channel
			}
		}
	}



	switch (settings.fmt)
	{
	case DXTn:
		if (has_alpha)
			ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);
		else
			ilSetInteger(IL_DXTC_FORMAT, IL_DXT1);
		break;
	case DXT1:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT1);
		break;
	case DXT3:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT3);
		break;
	case DXT5:
		ilSetInteger(IL_DXTC_FORMAT, IL_DXT5);
		break;
	}

	if (settings.mipmaps)
	{
		iluBuildMipmaps();
/*
		// TODO: replace with proper sharp mipmap code

		int num = ilGetInteger(IL_NUM_MIPMAPS);
		for (int n = 1; n < num; ++n)
		{
			ilActiveMipmap(n);
			iluSharpen(3.0, 1);
			ilActiveMipmap(0);
		}
*/
		check();
	}

	ilEnable(IL_FILE_OVERWRITE);

	check();

	if (! ilSaveImage((TCHAR*) filename_out.c_str()))
	{
		std::tstring msg = _T("Error saving file '") + filename_out + _T("' - aborting.");
		die(msg.c_str());
	}

	check();
}