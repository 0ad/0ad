#ifndef INCLUDED_CPLAYLIST
#define INCLUDED_CPLAYLIST

#include <vector>
#include <string>
#include <fstream>

class CPlayList
{
public:
	CPlayList();
	CPlayList(const char* file);
	~CPlayList();
	void Load(const char* file);
	void List();
	void Add(std::string name);

private:
	std::vector<std::string> tracks;
};

#endif
