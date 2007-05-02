#include "precompiled.h"

#include "CPlayList.h"
#include <stdio.h>	// sscanf
#include "lib/res/res.h"

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

void CPlayList::Load(const char* file)
{
	tracks.clear();

	FileIOBuf buf; size_t size;
	if(vfs_load(file, buf, size) < 0)
		return;

	const char* playlist = (const char*)buf;
	char track[512];

	while(sscanf(playlist, "%511s\n", track) == 1)
		tracks.push_back(track);

	(void)file_buf_free(buf);
}


void CPlayList::List()
{
	for(unsigned int i = 0; i < tracks.size(); i++)
	{
		debug_printf("%s\n", tracks.at(i).c_str());
	}
}

void CPlayList::Add(std::string name)
{
	tracks.push_back(name);
}
