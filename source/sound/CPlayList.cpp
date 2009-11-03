/* Copyright (C) 2009 Wildfire Games.
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

#include "CPlayList.h"

#include <stdio.h>	// sscanf
#include "ps/Filesystem.h"


CPlayList::CPlayList()
{
	tracks.clear();
}

CPlayList::CPlayList(const VfsPath& pathname)
{
	Load(pathname);
}

CPlayList::~CPlayList()
{

}

void CPlayList::Load(const VfsPath& pathname)
{
	tracks.clear();

	shared_ptr<u8> buf; size_t size;
	if(g_VFS->LoadFile(pathname, buf, size) < 0)
		return;

	const char* playlist = (const char*)buf.get();
	char track[512];

	while(sscanf(playlist, "%511s\n", track) == 1)
		tracks.push_back(CStrW(track));
}


void CPlayList::List()
{
	for(size_t i = 0; i < tracks.size(); i++)
	{
		debug_printf(L"%ls\n", tracks[i].c_str());
	}
}

void CPlayList::Add(std::wstring name)
{
	tracks.push_back(name);
}
