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

#ifndef INCLUDED_IATLASSERIALISER
#define INCLUDED_IATLASSERIALISER

#include "AtlasObject/AtlasObject.h"

// An interface for GUI components to transfer their contents to/from AtObjs.
// (Maybe "serialise" isn't quite correct, since AtObjs aren't serial, but
// it's close enough...)
class IAtlasSerialiser
{
public:
	// Freeze/Thaw are mainly used by the 'undo' system, to take a snapshot
	// of the a GUI component's state, and to revert to that snapshot.
	// obj.Thaw(obj.Freeze()) should leave the component's contents unchanged.

	virtual AtObj FreezeData()=0; // wxWindow defines a Freeze method, so we're stuck with an uglier name
	virtual void ThawData(AtObj& in)=0;

	// Import/Export have a different meaning to Freeze/Thaw: they handle
	// data that is read from / written to XML files. Import needs to be
	// backwards-compatible with old data files that might still be in use.
	// Export may perform some processing on the data to tidy it up.
	// (By default, they just call Freeze and Thaw, since that's often good enough)

	virtual void ImportData(AtObj& in) { ThawData(in); }
	virtual AtObj ExportData() { return FreezeData(); }
};

#endif // INCLUDED_IATLASSERIALISER
