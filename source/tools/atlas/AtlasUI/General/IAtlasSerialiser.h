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
