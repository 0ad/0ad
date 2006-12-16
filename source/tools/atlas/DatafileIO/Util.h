#include <string>
#include <cassert>

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#endif

namespace DatafileIO
{
#ifdef _WIN32
	// TODO: proper portability
	typedef int int32_t;
	typedef unsigned int uint32_t;
	typedef unsigned short uint16_t;
	typedef std::wstring utf16string;
	typedef wchar_t utf16_t;
#else
	typedef uint16_t utf16_t;
	typedef std::basic_string<utf16_t> utf16string;
#endif

	C_ASSERT(sizeof(int32_t) == 4);
	C_ASSERT(sizeof(uint32_t) == 4);
	C_ASSERT(sizeof(uint16_t) == 2);
	C_ASSERT(sizeof(utf16_t) == 2);

	class InputStream;
	class OutputStream;

	// Read/write 4-byte length + UCS-2 string
	utf16string ReadUString(InputStream& stream);
	void WriteUString(OutputStream& stream, const utf16string& string);

#ifdef _WIN32
# define utf16tow(_str) _str
# define wtoutf16(_str) _str
#else
	std::wstring utf16tow(const utf16string &str);
	utf16string wtoutf16(const std::wstring &str);
#endif
}
