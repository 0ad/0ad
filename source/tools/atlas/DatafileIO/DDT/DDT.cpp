#include "precompiled.h"

#include "DDT.h"

#include "Stream/Stream.h"
#include "Util.h"

#include "IL/il.h"
#include "IL/ilu.h"

#include <cassert>
#include <string.h>
#include <stdlib.h>

using namespace DatafileIO;

DDTFile::DDTFile(SeekableInputStream& stream)
: m_Stream(stream)
{
	// TODO: allow multiple nested DDTFiles, with ref-counted init
	ilInit();
	ilGenImages(1, &m_Image);
}

DDTFile::~DDTFile()
{
	ilDeleteImages(1, &m_Image);
	ilShutDown();
}


struct DDTImage
{
	int width, height;
	off_t offset;
	size_t length;
};

#ifdef USE_DEVIL_DXT
static void LoadDXT(int dxtType, unsigned char* oldData);
static void SaveDXT(int dxtType); // saves the currently bound image
static void SwizzleAGBR();
#endif
static void ToggleOrigin(); // urgh

bool DDTFile::Read(FileType type)
{
	ilBindImage(m_Image);

	if (type == DDT)
	{
		char head[4];
		m_Stream.Read(head, 4);
		if (strncmp(head, "RTS3", 4) != 0)
		{
			// TODO: report helpful error message
			return false;
		}

		char format[3];
		m_Stream.Read(format, 3);
		m_Type_Usage = (Type_Usage)format[0];
		m_Type_Alpha = (Type_Alpha)format[1];
		m_Type_Format = (Type_Format)format[2];

		char mipmapLevels;
		m_Stream.Read(&mipmapLevels, 1);
		m_Type_Levels = mipmapLevels;

		uint32_t baseWidth, baseHeight;
		m_Stream.Read(&baseWidth, 4);
		m_Stream.Read(&baseHeight, 4);

		int numImagesPerLevel = (m_Type_Usage == CUBE ? 6 : 1);
		int numImages = mipmapLevels * numImagesPerLevel;

		std::vector<DDTImage> Images;

		Images.resize(numImages);
		for (int i = 0; i < numImages; ++i)
		{
			int width = baseWidth >> (i/numImagesPerLevel); if (width < 1) width = 1;
			int height = baseHeight >> (i/numImagesPerLevel); if (height < 1) height = 1;
			uint32_t offset, length;
			m_Stream.Read(&offset, 4);
			m_Stream.Read(&length, 4);
			Images[i].width = width;
			Images[i].height = height;
			Images[i].offset = offset;
			Images[i].length = length;
		}

		// Read the first image. (TODO: cubemaps)

		int w = Images[0].width;
		int h = Images[0].height;
		ilTexImage(w,h,1,  4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);

		unsigned char* newData = (unsigned char*)ilGetData();
		switch (m_Type_Format)
		{
		case BGRA:
			{
				unsigned char* oldData = new unsigned char[w*h*4];
				m_Stream.Read(oldData, w*h*4);
				for (int i = 0; i < w*h; ++i)
				{
					newData[i*4+0] = oldData[i*4+2];
					newData[i*4+1] = oldData[i*4+1];
					newData[i*4+2] = oldData[i*4+0];
					newData[i*4+3] = oldData[i*4+3];
				}
				delete[] oldData;
			}
			break;

#ifdef USE_DEVIL_DXT
		case DXT1:
		case DXT3:
		case NORMSPEC:
			{
				int dxtType = (m_Type_Format == DXT1 ? 1 : m_Type_Format == DXT3 ? 3 : 5);
				int pixPerByte = (m_Type_Format == DXT1 ? 2 : 1);
				unsigned char* oldData = new unsigned char[w*h/pixPerByte];
				m_Stream.Read(oldData, w*h/pixPerByte);
				LoadDXT(dxtType, oldData);
				if (m_Type_Format == NORMSPEC)
					SwizzleAGBR();
				delete[] oldData;
			}
			break;
#endif

		case GREY:
			{
				unsigned char* oldData = new unsigned char[w*h];
				m_Stream.Read(oldData, w*h);
				for (int i = 0; i < w*h; ++i)
				{
					newData[i*4+0] =
					newData[i*4+1] =
					newData[i*4+2] = oldData[i];
					newData[i*4+3] = 255;
				}
				delete[] oldData;
			}
			break;

		default:
			//assert(! "Unhandled format");
			ilClearColour(1.0f, 0.0f, 1.0f, 1.0f);
			ilClearImage();
			break;
		}

		ToggleOrigin(); // use this instead of iluFlip because we don't want to change the actual data
	}

	else if (type == TGA)
	{
		void* buffer;
		size_t size;
		m_Stream.AcquireBuffer(buffer, size);
		ilLoadL(IL_TGA, buffer, (ILuint)size);
		m_Stream.ReleaseBuffer(buffer);
		ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
		iluFlipImage();
	}
	else
	{
		assert(! "Invalid type");
	}

	return true;
}


bool DDTFile::GetImageData(void*& buffer, int& width, int& height, bool realAlpha)
{
	ilBindImage(m_Image);

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	if (realAlpha)
	{
		buffer = malloc(width*height * 4);
		memcpy(buffer, ilGetData(), width*height * 4);
	}
	else
	{
		buffer = malloc(width*height * 3 * 2);
		unsigned char* newData = (unsigned char*)buffer;
		unsigned char* oldData = (unsigned char*)ilGetData();

		for (int i = 0; i < width*height; ++i)
		{
			newData[i*3+0] = oldData[i*4+0];
			newData[i*3+1] = oldData[i*4+1];
			newData[i*3+2] = oldData[i*4+2];
		}
		for (int i = 0; i < width*height; ++i)
		{
			newData[(i+width*height)*3+0] =
				newData[(i+width*height)*3+1] =
				newData[(i+width*height)*3+2] = oldData[i*4+3];
		}
	}

	height *= 2;

	return true;

}

//////////////////////////////////////////////////////////////////////////

// DevIL code: (slightly nasty, since DevIL doesn't seem to be flexible enough
// to do what I need it to do...)

struct ILOutputStream
{
	static OutputStream* stream;
	static ILHANDLE ILAPIENTRY Open(const ILstring)
	{
		return (void*)-1;
	}
	static ILvoid ILAPIENTRY Close(ILHANDLE)
	{
	}
	static ILint ILAPIENTRY Putc(ILubyte c, ILHANDLE)
	{
		stream->Write(&c, 1);
		return c;
	}
	static ILint ILAPIENTRY Seek(ILHANDLE, ILint /*offset*/, ILint /*whence*/)
	{
		assert(! "Not implemented");
		return 0;
	}
	static ILint ILAPIENTRY Tell(ILHANDLE)
	{
		return stream->Tell();
	}
	static ILint ILAPIENTRY Write(const void* data, ILuint size, ILuint count, ILHANDLE)
	{
		if (size*count)
			stream->Write(data, size*count);
		return count;
	}
};
OutputStream* ILOutputStream::stream = NULL;

extern "C" {
	extern ILboolean ilSaveTargaF(ILHANDLE File);
		// because DevIL doesn't want to write to things that aren't
		// really files, so we have to use its internal writing functions
	extern ILvoid iSetOutputFile(ILHANDLE File);
}
static void ToggleOrigin(); // urgh

bool DDTFile::SaveFile(OutputStream& stream, FileType outputType)
{
	ilBindImage(m_Image);

	ilSetWrite(&ILOutputStream::Open, &ILOutputStream::Close, &ILOutputStream::Putc,
		&ILOutputStream::Seek, &ILOutputStream::Tell, &ILOutputStream::Write);
	iSetOutputFile(NULL); // make sure it's using the right output functions

	ILOutputStream::stream = &stream;

	if (outputType == TGA)
	{
		ilSaveTargaF(NULL);
	}
	else if (outputType == DDT)
	{
		int bpp;
		switch (m_Type_Format)
		{
		case BGRA: bpp = 32; break;
		case GREY: bpp = 8; break;
		case DXT1: bpp = 4; break;
		case DXT3: bpp = 8; break;
		case NORMSPEC: bpp = 8; break;
		default: assert(! "Invalid format"); return false;
		}

		stream.Write("RTS3", 4);

		char format[4];
		format[0] = (char)m_Type_Usage;
		format[1] = (char)m_Type_Alpha;
		format[2] = (char)m_Type_Format;
		format[3] = (char)m_Type_Levels;
		stream.Write(format, 4);

		uint32_t baseWidth, baseHeight;
		baseWidth = ilGetInteger(IL_IMAGE_WIDTH);
		baseHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		stream.Write(&baseWidth, 4);
		stream.Write(&baseHeight, 4);

		int numImagesPerLevel = 1; // TODO: cubemaps
		int numImages = m_Type_Levels * numImagesPerLevel;
		uint32_t imgOffset = 16 + 8*numImages;

		for (int i = 0; i < numImages; ++i)
		{
			int width = baseWidth >> (i/numImagesPerLevel); if (width < 1) width = 1;
			int height = baseHeight >> (i/numImagesPerLevel); if (height < 1) height = 1;
			uint32_t length = width*height * bpp / 8;
			stream.Write(&imgOffset, 4);
			stream.Write(&length, 4);
			imgOffset += length;
		}

		for (int i = 0; i < numImages; ++i)
		{
			int width = baseWidth >> (i/numImagesPerLevel); if (width < 1) width = 1;
			int height = baseHeight >> (i/numImagesPerLevel); if (height < 1) height = 1;

			ilBindImage(m_Image);

			ILuint img = ilCloneCurImage();
			ilBindImage(img);
			iluImageParameter(ILU_FILTER, ILU_SCALE_BOX); // TODO - proper mipmapping
			iluScale(width, height, 1);

			switch (m_Type_Format)
			{
			case BGRA:
				{
					unsigned char* newData = new unsigned char[width*height*4];
					unsigned char* oldData = (unsigned char*)ilGetData();
					for (int i = 0; i < width*height; ++i)
					{
						newData[i*4+0] = oldData[i*4+2];
						newData[i*4+1] = oldData[i*4+1];
						newData[i*4+2] = oldData[i*4+0];
						newData[i*4+3] = oldData[i*4+3];
					}
					stream.Write(newData, width*height*4);
					delete[] newData;
					break;
				}
			case GREY:
				{
					unsigned char* newData = new unsigned char[width*height];
					unsigned char* oldData = (unsigned char*)ilGetData();
					for (int i = 0; i < width*height; ++i)
					{
						newData[i] = oldData[i*4+0];
					}
					stream.Write(newData, width*height);
					delete[] newData;
					break;
				}
#ifdef USE_DEVIL_DXT
			case DXT1:
				SaveDXT(1);
				break;
			case DXT3:
				SaveDXT(3);
				break;
			case NORMSPEC:
				SwizzleAGBR();
				SaveDXT(5);
				break;
#endif
			}

			ilDeleteImages(1, &img);
		}
	}

	ilResetWrite();

	return true;
}



// Evilness:
#include "IL/devil_internal_exports.h"
extern "C"
{
	extern ILboolean DecompressDXT1();
	extern ILboolean DecompressDXT3();
	extern ILboolean DecompressDXT5();
	extern ILuint Compress(ILimage* Image, ILenum DXTCFormat);
	extern ILimage* iCurImage;
}

static void ToggleOrigin()
{
	iCurImage->Origin = (iCurImage->Origin == IL_ORIGIN_UPPER_LEFT ? IL_ORIGIN_LOWER_LEFT : IL_ORIGIN_UPPER_LEFT);
}

#ifdef USE_DEVIL_DXT
extern "C"
{
	extern ILubyte* CompData;
	extern ILint Depth, Width, Height;
	extern ILimage* Image;
}

static void LoadDXT(int dxtType, unsigned char* oldData)
{
	// More evilness, that assumes a lot about DevIL's internals:
	CompData = (ILubyte*)oldData;
	Image = iCurImage;
	Width = Image->Width;
	Height = Image->Height;
	Depth = Image->Depth;

	switch (dxtType)
	{
	case 1: DecompressDXT1(); break;
	case 3: DecompressDXT3(); break;
	case 5: DecompressDXT5(); break;
	default: assert(0);
	}

	CompData = NULL;
	Image = NULL;
}

static void SwizzleAGBR()
{
	ILubyte* data = ilGetData();
	ILint pixels = ilGetInteger(IL_IMAGE_WIDTH)*ilGetInteger(IL_IMAGE_HEIGHT);
	for (ILint i = 0; i < pixels; ++i)
	{
		ILubyte t = data[i*4+0];
		data[i*4+0] = data[i*4+3];
		data[i*4+3] = t;
	}
}

static void SaveDXT(int dxtType)
{
	Compress(ilGetCurImage(), dxtType==1 ? IL_DXT1 : dxtType==3 ? IL_DXT3 : IL_DXT5);
}

#endif
