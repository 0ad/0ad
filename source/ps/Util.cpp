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
#include "lib/allocators/shared_ptr.h"
#include "lib/posix/posix_utsname.h"
#include "lib/sysdep/cpu.h"
#include "lib/sysdep/os_cpu.h"
#include "lib/sysdep/smbios.h"
#include "lib/sysdep/sysdep.h"	// sys_OpenFile
#include "lib/tex/tex.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Pyrogenesis.h"
#include "ps/VideoMode.h"
#include "renderer/backend/gl/Device.h"

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

	debug_printf("FILES| Hardware details written to %s\n", pathname.string8().c_str());
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
