#include "precompiled.h"

#include "CPlayList.h"

#include <stdio.h>	// sscanf
#include "ps/Filesystem.h"

CPlayList::CPlayList(void)
{
	tracks.clear();
}

CPlayList::CPlayList(const char* file)
{
	Load(file);
}

CPlayList::~CPlayList(void)
{

}

void CPlayList::Load(const char* filename)
{
	tracks.clear();

	shared_ptr<u8> buf; size_t size;
	if(g_VFS->LoadFile(filename, buf, size) < 0)
		return;

	const char* playlist = (const char*)buf.get();
	char track[512];

	while(sscanf(playlist, "%511s\n", track) == 1)
		tracks.push_back(track);
}


void CPlayList::List()
{
	for(size_t i = 0; i < tracks.size(); i++)
	{
		debug_printf("%s\n", tracks.at(i).c_str());
	}
}

void CPlayList::Add(std::string name)
{
	tracks.push_back(name);
}
