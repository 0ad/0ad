/* Copyright (C) 2023 Wildfire Games.
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

#include "lib/allocators/shared_ptr.h"
#include "lib/tex/tex.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Pyrogenesis.h"

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

CStr GetStatusAsString(Status status)
{
	return utf8_from_wstring(ErrorString(status));
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
