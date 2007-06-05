// Precompiled headers:

#ifdef _MSC_VER
# define _SCL_SECURE_NO_DEPRECATE // shut up, std::copy isn't deprecated
#endif

#ifdef USING_PCH

// Exclude rarely-used stuff from Windows headers
#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
#endif

#include <vector>
#include <string>
#include <set>
#include <stack>
#include <map>

#endif // USING_PCH
