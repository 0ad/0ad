#include "AtlasObject/AtlasObject.h"

class AtlasClipboard
{
public:
	// Return true on success
	static bool SetClipboard(AtObj& in);
	static bool GetClipboard(AtObj& out);
};