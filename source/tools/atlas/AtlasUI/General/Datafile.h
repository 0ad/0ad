#include "AtlasObject/AtlasObject.h"

class Datafile
{
public:

	// Read configuration data from data/tools/atlas/lists.xml
	static AtObj ReadList(const char* section);

	// Specify the location of .../binaries/system, as an absolute path, or
	// relative to the current working directory.
	static void SetSystemDirectory(const wxString& dir);

private:
	static wxString systemDir;
};