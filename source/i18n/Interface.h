/*

The only file that external code should need to include.

*/

#ifndef I18N_INTERFACE_H
#define I18N_INTERFACE_H

#include "StringBuffer.h"
#include "DataTypes.h"

struct JSContext;
struct JSObject;

namespace I18n
{
	// Use an interface class, so minimal headers are required by
	// anybody who only wants to make use of Translate()
	class CLocale_interface
	{
	public:
		virtual StringBuffer Translate(const wchar_t* id) = 0;

		// Load* functions return true for success, false for failure

		// Pass the contents of a UTF-16LE BOMmed .js file.
		// The filename is just used to give more useful error messages from JS.
		virtual bool LoadFunctions(const char* filedata, size_t len, const char* filename) = 0;

		// Desires a .lng file, as produced by convert.pl
		virtual bool LoadStrings(const char* filedata) = 0;

		// Needs .wrd files generated through tables.pl
		virtual bool LoadDictionary(const char* filedata) = 0;

		virtual ~CLocale_interface() {}
	};

	// Build a CLocale. Returns NULL on failure.
	CLocale_interface* NewLocale(JSContext* cx, JSObject* scope);
}

#endif // I18N_INTERFACE_H
