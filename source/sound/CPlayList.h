#ifndef CPLAYLIST_H
#define CPLAYLIST_H

#include <vector>
#include <string>
#include <fstream>

class CPlayList
{
public:
	CPlayList();
	CPlayList(const char* file);
	~CPlayList();
	void load(const char* file);
	void list();
	void add(std::string name);

private:
	std::vector<std::string> tracks;
};

#endif
