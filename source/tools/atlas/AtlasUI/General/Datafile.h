/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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

	// Specify the location of .../binaries/data, as an absolute path, or
	// relative to the current working directory.
	void SetDataDirectory(const wxString& dir);

	// Returns the location of .../binaries/data. (TODO (eventually): replace
	// this with a proper VFS-aware system.)
	wxString GetDataDirectory();

	// Returns a list of files matching the given wildcard (* and ?) filter
	// inside .../binaries/data/<dir>, not recursively.
	wxArrayString EnumerateDataFiles(const wxString& dir, const wxString& filter);
}
