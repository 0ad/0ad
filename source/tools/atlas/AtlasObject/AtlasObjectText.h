// Public interface to text-related functions that make use of std::wstring
//
// (This is done in order to avoid forcing inclusion of all the
// STL headers when they're not needed)

#ifdef new // HACK: to make the STL headers happy with a redefined 'new'
# undef new
# include <string>
# define new new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#else
# include <string>
#endif

class AtObj;

namespace AtlasObject
{
	// Generate a human-readable string representation of the AtObj,
	// as an easy way of visualising the data (without any horridly ugly
	// XML junk)
	std::wstring ConvertToString(const AtObj& obj);
}
