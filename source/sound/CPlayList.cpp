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
	load(file);
}

CPlayList::~CPlayList(void)
{

}

void CPlayList::load(const char* file)
{
	tracks.clear();

	void* p;
	size_t size;
	Handle hm = vfs_load(file, p, size);
	if(hm <= 0)
		throw "CPlaylist::Load failed";

	const char* playlist = (const char*)p;
	const char* track;
	debug_warn("TODO: This code looks quite suspicious");
	while(sscanf(playlist, "%s\n", &track) == 1)
		tracks.push_back(track);

	mem_free_h(hm);
}


void CPlayList::list()
{
	for(unsigned int i = 0; i < tracks.size(); i++)
	{
		debug_printf("%s\n", tracks.at(i).c_str());
	}
}

void CPlayList::add(std::string name)
{
	tracks.push_back(name);
}
