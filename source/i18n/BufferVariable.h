/*

BufferVariable stores the parameters that have been passed to a StringBuffer,
providing hashes and strings on request.

*/

#ifndef INCLUDED_I18N_BUFFERVARIABLE
#define INCLUDED_I18N_BUFFERVARIABLE

#include "Common.h"

#include "StrImmutable.h"



namespace I18n
{
	class CLocale;

	enum {
		vartype_int,
		vartype_double,
		vartype_string,
		vartype_rawstring // won't be translated automatically
	};

	class BufferVariable
	{
	public:
		char Type;
		virtual StrImW ToString(CLocale*) = 0;
		virtual u32 Hash() = 0;
		virtual ~BufferVariable() {};
	};

	// Factory constructor type thing, sort of
	template<typename T>
		BufferVariable* NewBufferVariable(T v);

	class BufferVariable_int : public BufferVariable
	{
	public:
		int value;
		BufferVariable_int(int v) : value(v) { Type = vartype_int; }
		StrImW ToString(CLocale*);
		u32 Hash();
		// Equality testing is required by the cache
		bool operator== (BufferVariable_int&);
	};

	class BufferVariable_double : public BufferVariable
	{
	public:
		double value;
		BufferVariable_double(double v) : value(v) { Type = vartype_double; }
		StrImW ToString(CLocale*);
		u32 Hash();
		bool operator== (BufferVariable_double&);
	};

	class BufferVariable_string : public BufferVariable
	{
	public:
		StrImW value;
		BufferVariable_string(StrImW v) : value(v) { Type = vartype_string; }
		StrImW ToString(CLocale*);
		u32 Hash();
		bool operator== (BufferVariable_string&);
	};

	class BufferVariable_rawstring : public BufferVariable
	{
	public:
		StrImW value;
		BufferVariable_rawstring(StrImW v) : value(v) { Type = vartype_rawstring; }
		StrImW ToString(CLocale*);
		u32 Hash();
		bool operator== (BufferVariable_rawstring&);
	};

	// BufferVariable==BufferVariable compares their Types, and then
	// uses the appropriate operator== if they're the same
	bool operator== (BufferVariable&, BufferVariable&);
}

#endif // INCLUDED_I18N_BUFFERVARIABLE
