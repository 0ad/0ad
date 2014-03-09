#ifndef __cxxtest__Win32ODSPrinter_h__
#define __cxxtest__Win32ODSPrinter_h__

#include <cxxtest/ParenPrinter.h>

#include "lib/sysdep/os/win/win.h"

namespace CxxTest 
{
	class ods_ostreambuf : public std::streambuf
	{
		std::streamsize xsputn(const char* s, std::streamsize n)
		{
			char* buf = new char[n+1];
			memcpy(buf, s, n);
			buf[n] = '\0';
			OutputDebugStringA(buf);
			delete[] buf;
			return n;
		}
	};

	class Win32ODSPrinter : public ParenPrinter
	{
		ods_ostreambuf buf;
		std::ostream ostr;
	public:
		Win32ODSPrinter() : buf(), ostr(&buf), ParenPrinter(ostr) {}
	};
}

#endif // __cxxtest__Win32ODSPrinter_h__