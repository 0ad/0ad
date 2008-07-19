/*

CLocale contains all the data about a locale (/language).
LoadFunctions/LoadStrings/LoadDictionary are used to specify data to be used
when translating, and Translate is used to perform translations.

All other methods are used internally by other I18n components.

*/

#ifndef INCLUDED_I18N_CLOCALE
#define INCLUDED_I18N_CLOCALE

#include "Common.h"

#include "Interface.h"
#include "StringBuffer.h"
#include "StrImmutable.h"
#include "ScriptInterface.h"

#include "lib/sysdep/stl.h" // STL_HASH_MAP

#include <map>
#include <algorithm>

struct JSContext;

namespace I18n
{

	class CLocale : public CLocale_interface
	{
		friend class StringBuffer;

	private:
		typedef STL_HASH_MAP<Str, TranslatedString*> StringsType;

	public:
		StringBuffer Translate(const wchar_t* id);

		bool LoadFunctions(const char* filedata, size_t len, const char* filename);
		bool LoadStrings(const char* filedata);
		bool LoadDictionary(const char* filedata);
		void UnloadDictionaries();

		const StrImW CallFunction(const char* name, const std::vector<BufferVariable*>& vars, const std::vector<ScriptValue*>& params);

	private:	
		struct DictData;
	public:
		typedef std::pair<const DictData*, const std::vector<Str>*> LookupType;
		
		// Returns a new'ed structure. Please remember to delete it.
		const LookupType* LookupWord(const Str& dictname, const Str& word);

		bool LookupProperty(const LookupType* data, const Str& property, Str& result);

		// Disable the "'this' used in base member initializer list" warning: only the
		// pointer (and not the data it points to) is accessed by ScriptObject's
		// constructor, so it shouldn't cause any problems.
#if MSC_VERSION
# pragma warning (disable: 4355)
#endif
		CLocale(JSContext* context, JSObject* scope) : CacheAge(0), Script(this, context, scope) {}
#if MSC_VERSION
# pragma warning (default: 4355)
#endif

		~CLocale();

		void AddToCache(StringBuffer*, Str&);
		bool ReadCached(StringBuffer*, Str&);
		void ClearCache();

	private:

		TranslatedString& AddDefaultString(const wchar_t* id);

		StringsType Strings;

		// Incremented by every call to Translate(), with
		// ClearCache() being run after a certain number of calls.
		// TODO: Replace with a better caching system.
		int CacheAge;
		static const int CacheAgeLimit = 1024;

		struct CacheData
		{
			u32 hash;
			std::vector<BufferVariable*> vars;
			Str output;
		};
		std::map<TranslatedString*, CacheData> TranslationCache;

		struct DictData
		{
			std::map< Str /* property name */, int /* property id */ > DictProperties;
			std::map< Str /* word */, std::vector<Str> /* properties */ > DictWords;
		};
		std::map<Str /* dictionary name */, DictData> Dictionaries;

		ScriptObject Script;

	};
}

#endif // INCLUDED_I18N_CLOCALE
