#ifndef CPLAYLIST_H
#define CPLAYLIST_H

#include <vector>
#include <string>
#include <fstream>

class CPlayList
{
public:
	CPlayList(void);
	CPlayList(char *file);
	~CPlayList(void);
	void load(char *file);
	void list();
	void add(std::string name);

private:
	std::vector<std::string> tracks;
};

#endif
