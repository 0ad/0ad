#include "precompiled.h"

#include "CPlayList.h"
#include <iostream>

CPlayList::CPlayList(void)
{
	tracks.clear();
}

CPlayList::CPlayList(char *file)
{
	load(file);
}

CPlayList::~CPlayList(void)
{

}

void CPlayList::load(char *file)
{
	std::ifstream input(file);

	tracks.clear();

	if(input.is_open())
	{
		std::string temp;

		while(getline(input,temp))
		{
			tracks.push_back(temp);
		}
		input.close();
	}
	else
	{
		std::cout << "Error opening file.";
	}
}


void CPlayList::list()
{
	for(unsigned int i = 0; i < tracks.size(); i++)
	{
		std::cout << tracks.at(i) << std::endl;
	}
}

void CPlayList::add(std::string name)
{
	tracks.push_back(name);
}
