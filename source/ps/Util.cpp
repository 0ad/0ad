/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "ps/Util.h"

#include "graphics/GameView.h"
#include "i18n/L10n.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/bits.h"	// round_up
#include "lib/ogl.h"
#include "lib/posix/posix_utsname.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/smbios.h"
#include "lib/sysdep/sysdep.h"	// sys_OpenFile
#include "lib/tex/tex.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Pyrogenesis.h"
#include "ps/VideoMode.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"

#if CONFIG2_AUDIO
#include "soundmanager/SoundManager.h"
#endif

#include <iomanip>
#include <sstream>

void WriteSystemInfo()
{
	TIMER(L"write_sys_info");
	struct utsname un;
	uname(&un);

	OsPath pathname = psLogDir()/"system_info.txt";
	FILE* f = sys_OpenFile(pathname, "w");
	if(!f)
		return;

	// current timestamp (redundant WRT OS timestamp, but that is not
	// visible when people are posting this file's contents online)
	{
	wchar_t timestampBuf[100] = {'\0'};
	time_t seconds;
	time(&seconds);
	struct tm* t = gmtime(&seconds);
	const size_t charsWritten = wcsftime(timestampBuf, ARRAY_SIZE(timestampBuf), L"(generated %Y-%m-%d %H:%M:%S UTC)", t);
	ENSURE(charsWritten != 0);
	fprintf(f, "%ls\n\n", timestampBuf);
	}

	// OS
	fprintf(f, "OS             : %s %s (%s)\n", un.sysname, un.release, un.version);

	// CPU
	fprintf(f, "CPU            : %s, %s", un.machine, cpu_IdentifierString());
	double cpuClock = os_cpu_ClockFrequency();	// query OS (may fail)
#if ARCH_X86_X64
	if(cpuClock <= 0.0)
		cpuClock = x86_x64::ClockFrequency();	// measure (takes a few ms)
#endif
	if(cpuClock > 0.0)
	{
		if(cpuClock < 1e9)
			fprintf(f, ", %.2f MHz\n", cpuClock*1e-6);
		else
			fprintf(f, ", %.2f GHz\n", cpuClock*1e-9);
	}
	else
		fprintf(f, "\n");

	// memory
	fprintf(f, "Memory         : %u MiB; %u MiB free\n", (unsigned)os_cpu_MemorySize(), (unsigned)os_cpu_MemoryAvailable());

	// graphics
	fprintf(f, "Video Card     : %s\n", g_VideoMode.GetBackendDevice()->GetName().c_str());
	fprintf(f, "Video Driver   : %s\n", g_VideoMode.GetBackendDevice()->GetDriverInformation().c_str());
	fprintf(f, "Video Mode     : %dx%d:%d\n", g_VideoMode.GetXRes(), g_VideoMode.GetYRes(), g_VideoMode.GetBPP());

#if CONFIG2_AUDIO
	if (g_SoundManager)
	{
		fprintf(f, "Sound Card     : %s\n", g_SoundManager->GetSoundCardNames().c_str());
		fprintf(f, "Sound Drivers  : %s\n", g_SoundManager->GetOpenALVersion().c_str());
	}
	else if(g_DisableAudio)
		fprintf(f, "Sound          : Game was ran without audio\n");
	else
		fprintf(f, "Sound          : No audio device was found\n");
#else
	fprintf(f, "Sound          : Game was compiled without audio\n");
#endif

	// OpenGL extensions (write them last, since it's a lot of text)
	fprintf(f, "\nBackend Extensions:\n");
	if (g_VideoMode.GetBackendDevice()->GetExtensions().empty())
		fprintf(f, "{unknown}\n");
	else
		for (const std::string& extension : g_VideoMode.GetBackendDevice()->GetExtensions())
			fprintf(f, "%s\n", extension.c_str());

	// System Management BIOS (even more text than OpenGL extensions)
	std::string smbios = SMBIOS::StringizeStructures(SMBIOS::GetStructures());
	fprintf(f, "\nSMBIOS: \n%s\n", smbios.c_str());

	fclose(f);
	f = 0;
}


// not thread-safe!
static const wchar_t* HardcodedErrorString(int err)
{
	static wchar_t description[200];
	StatusDescription((Status)err, description, ARRAY_SIZE(description));
	return description;
}

// not thread-safe!
const wchar_t* ErrorString(int err)
{
	// language file not available (yet)
	return HardcodedErrorString(err);

	// TODO: load from language file
}



// write the specified texture to disk.
// note: <t> cannot be made const because the image may have to be
// transformed to write it out in the format determined by <fn>'s extension.
Status tex_write(Tex* t, const VfsPath& filename)
{
	DynArray da;
	RETURN_STATUS_IF_ERR(t->encode(filename.Extension(), &da));

	// write to disk
	Status ret = INFO::OK;
	{
		std::shared_ptr<u8> file = DummySharedPtr(da.base);
		const ssize_t bytes_written = g_VFS->CreateFile(filename, file, da.pos);
		if(bytes_written > 0)
			ENSURE(bytes_written == (ssize_t)da.pos);
		else
			ret = (Status)bytes_written;
	}

	ignore_result(da_free(&da));
	return ret;
}

/**
 * Return an unused directory, based on date and index (for example 2016-02-09_0001)
 */
OsPath createDateIndexSubdirectory(const OsPath& parentDir)
{
    const std::time_t timestamp = std::time(nullptr);
    const struct std::tm* now = std::localtime(&timestamp);

	// Two processes executing this simultaneously might attempt to create the same directory.
	int tries = 0;
	const int maxTries = 10;

	int i = 0;
	OsPath path;
	char directory[256];

	do
	{
		sprintf(directory, "%04d-%02d-%02d_%04d", now->tm_year+1900, now->tm_mon+1, now->tm_mday, ++i);
		path = parentDir / CStr(directory);

		if (DirectoryExists(path) || FileExists(path))
			continue;

		if (CreateDirectories(path, 0700, ++tries > maxTries) == INFO::OK)
			break;

	} while(tries <= maxTries);

	return path;
}

static size_t s_nextScreenshotNumber;

// <extension> identifies the file format that is to be written
// (case-insensitive). examples: "bmp", "png", "jpg".
// BMP is good for quick output at the expense of large files.
void WriteScreenshot(const VfsPath& extension)
{
	// get next available numbered filename
	// note: %04d -> always 4 digits, so sorting by filename works correctly.
	const VfsPath basenameFormat(L"screenshots/screenshot%04d");
	const VfsPath filenameFormat = basenameFormat.ChangeExtension(extension);
	VfsPath filename;
	vfs::NextNumberedFilename(g_VFS, filenameFormat, s_nextScreenshotNumber, filename);

	const size_t w = (size_t)g_xres, h = (size_t)g_yres;
	const size_t bpp = 24;
	GLenum fmt = GL_RGB;
	int flags = TEX_BOTTOM_UP;
	// we want writing BMP to be as fast as possible,
	// so read data from OpenGL in BMP format to obviate conversion.
	if(extension == L".bmp")
	{
#if !CONFIG2_GLES // GLES doesn't support BGR
		fmt = GL_BGR;
		flags |= TEX_BGR;
#endif
	}

	// Hide log messages and re-render
	RenderLogger(false);
	Render();
	RenderLogger(true);

	const size_t img_size = w * h * bpp/8;
	const size_t hdr_size = tex_hdr_size(filename);
	std::shared_ptr<u8> buf;
	AllocateAligned(buf, hdr_size+img_size, maxSectorSize);
	GLvoid* img = buf.get() + hdr_size;
	Tex t;
	if(t.wrap(w, h, bpp, flags, buf, hdr_size) < 0)
		return;
	glReadPixels(0, 0, (GLsizei)w, (GLsizei)h, fmt, GL_UNSIGNED_BYTE, img);

	if (tex_write(&t, filename) == INFO::OK)
	{
		OsPath realPath;
		g_VFS->GetRealPath(filename, realPath);

		LOGMESSAGERENDER(g_L10n.Translate("Screenshot written to '%s'"), realPath.string8());

		debug_printf(
			CStr(g_L10n.Translate("Screenshot written to '%s'") + "\n").c_str(),
			realPath.string8().c_str());
	}
	else
		LOGERROR("Error writing screenshot to '%s'", filename.string8());
}



// Similar to WriteScreenshot, but generates an image of size tileWidth*tiles x tileHeight*tiles.
void WriteBigScreenshot(const VfsPath& extension, int tiles, int tileWidth, int tileHeight)
{
	// If the game hasn't started yet then use WriteScreenshot to generate the image.
	if (g_Game == nullptr)
	{
		WriteScreenshot(L".bmp");
		return;
	}

	// get next available numbered filename
	// note: %04d -> always 4 digits, so sorting by filename works correctly.
	const VfsPath basenameFormat(L"screenshots/screenshot%04d");
	const VfsPath filenameFormat = basenameFormat.ChangeExtension(extension);
	VfsPath filename;
	vfs::NextNumberedFilename(g_VFS, filenameFormat, s_nextScreenshotNumber, filename);

	// Slightly ugly and inflexible: Always draw 640*480 tiles onto the screen, and
	// hope the screen is actually large enough for that.
	ENSURE(g_xres >= tileWidth && g_yres >= tileHeight);

	const int img_w = tileWidth * tiles, img_h = tileHeight * tiles;
	const int bpp = 24;
	GLenum fmt = GL_RGB;
	int flags = TEX_BOTTOM_UP;
	// we want writing BMP to be as fast as possible,
	// so read data from OpenGL in BMP format to obviate conversion.
	if(extension == L".bmp")
	{
#if !CONFIG2_GLES // GLES doesn't support BGR
		fmt = GL_BGR;
		flags |= TEX_BGR;
#endif
	}

	const size_t img_size = img_w * img_h * bpp/8;
	const size_t tile_size = tileWidth * tileHeight * bpp/8;
	const size_t hdr_size = tex_hdr_size(filename);
	void* tile_data = malloc(tile_size);
	if(!tile_data)
	{
		WARN_IF_ERR(ERR::NO_MEM);
		return;
	}
	std::shared_ptr<u8> img_buf;
	AllocateAligned(img_buf, hdr_size + img_size, maxSectorSize);

	Tex t;
	GLvoid* img = img_buf.get() + hdr_size;
	if(t.wrap(img_w, img_h, bpp, flags, img_buf, hdr_size) < 0)
	{
		free(tile_data);
		return;
	}

	ogl_WarnIfError();

	CCamera oldCamera = *g_Game->GetView()->GetCamera();

	// Resize various things so that the sizes and aspect ratios are correct
	{
		g_Renderer.Resize(tileWidth, tileHeight);
		SViewPort vp = { 0, 0, tileWidth, tileHeight };
		g_Game->GetView()->SetViewport(vp);
	}

#if !CONFIG2_GLES
	// Temporarily move everything onto the front buffer, so the user can
	// see the exciting progress as it renders (and can tell when it's finished).
	// (It doesn't just use SwapBuffers, because it doesn't know whether to
	// call the SDL version or the Atlas version.)
	GLint oldReadBuffer, oldDrawBuffer;
	glGetIntegerv(GL_READ_BUFFER, &oldReadBuffer);
	glGetIntegerv(GL_DRAW_BUFFER, &oldDrawBuffer);
	glDrawBuffer(GL_FRONT);
	glReadBuffer(GL_FRONT);
#endif

	// Render each tile
	CMatrix3D projection;
	projection.SetIdentity();
	const float aspectRatio = 1.0f * tileWidth / tileHeight;
	for (int tile_y = 0; tile_y < tiles; ++tile_y)
	{
		for (int tile_x = 0; tile_x < tiles; ++tile_x)
		{
			// Adjust the camera to render the appropriate region
			if (oldCamera.GetProjectionType() == CCamera::ProjectionType::PERSPECTIVE)
			{
				projection.SetPerspectiveTile(oldCamera.GetFOV(), aspectRatio, oldCamera.GetNearPlane(), oldCamera.GetFarPlane(), tiles, tile_x, tile_y);
			}
			g_Game->GetView()->GetCamera()->SetProjection(projection);

			RenderLogger(false);
			RenderGui(false);
			Render();
			RenderGui(true);
			RenderLogger(true);

			// Copy the tile pixels into the main image
			glReadPixels(0, 0, tileWidth, tileHeight, fmt, GL_UNSIGNED_BYTE, tile_data);
			for (int y = 0; y < tileHeight; ++y)
			{
				void* dest = static_cast<char*>(img) + ((tile_y * tileHeight + y) * img_w + (tile_x * tileWidth)) * bpp / 8;
				void* src = static_cast<char*>(tile_data) + y * tileWidth * bpp / 8;
				memcpy(dest, src, tileWidth * bpp / 8);
			}
		}
	}

#if !CONFIG2_GLES
	// Restore the buffer settings
	glDrawBuffer(oldDrawBuffer);
	glReadBuffer(oldReadBuffer);
#endif

	// Restore the viewport settings
	{
		g_Renderer.Resize(g_xres, g_yres);
		SViewPort vp = { 0, 0, g_xres, g_yres };
		g_Game->GetView()->SetViewport(vp);
		g_Game->GetView()->GetCamera()->SetProjectionFromCamera(oldCamera);
	}

	if (tex_write(&t, filename) == INFO::OK)
	{
		OsPath realPath;
		g_VFS->GetRealPath(filename, realPath);

		LOGMESSAGERENDER(g_L10n.Translate("Screenshot written to '%s'"), realPath.string8());

		debug_printf(
			CStr(g_L10n.Translate("Screenshot written to '%s'") + "\n").c_str(),
			realPath.string8().c_str());
	}
	else
		LOGERROR("Error writing screenshot to '%s'", filename.string8());

	free(tile_data);
}

std::string Hexify(const std::string& s)
{
	std::stringstream str;
	str << std::hex;
	for (const char& c : s)
		str << std::setfill('0') << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
	return str.str();
}

std::string Hexify(const u8* s, size_t length)
{
	std::stringstream str;
	str << std::hex;
	for (size_t i = 0; i < length; ++i)
		str << std::setfill('0') << std::setw(2) << static_cast<int>(s[i]);
	return str.str();
}
