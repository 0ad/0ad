#include "AtlasObject/AtlasObject.h"

class Datafile
{
public:

	// Read configuration data from data/tools/atlas/lists.xml
	static AtObj ReadList(const char* section);
};