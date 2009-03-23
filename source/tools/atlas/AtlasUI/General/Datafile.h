#include "AtlasObject/AtlasObject.h"

namespace Datafile
{
	// Read configuration data from data/tools/atlas/lists.xml
	AtObj ReadList(const char* section);

	// Read an entire file. Returns true on success.
	bool SlurpFile(const wxString& filename, std::string& out);

	// Specify the location of .../binaries/system, as an absolute path, or
	// relative to the current working directory.
	void SetSystemDirectory(const wxString& dir);

	// Returns the location of .../binaries/data. (TODO (eventually): replace
	// this with a proper VFS-aware system.)
	wxString GetDataDirectory();

	// Returns a list of files matching the given wildcard (* and ?) filter
	// inside .../binaries/data/<dir>, not recursively.
	wxArrayString EnumerateDataFiles(const wxString& dir, const wxString& filter);
}
