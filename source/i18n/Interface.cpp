/*

Vague overview of the i18n code:
(TODO: Improve the documentation)

CLocale stores all locale-specific (locale==language, roughly) data.
Usually there's only going to be one in existence.

A language needs to define:

* String files that define translations for phrases.
* Dictionary files that translate individual words (usually names of objects),
  including extra information that the grammar requires (whether to use 'a' or
  'an' in English, gender in lots of European languages, etc)
* .js files containing functions that apply grammatical rules
  (e.g. choosing singular vs plural depending on a number)

CLocale::LoadStrings / LoadDictionary / LoadFunctions are used to input the
data from the appropriate files. Call multiple times if desired.

CLocale::Translate is the primary interface. Pass it a unique identifier
string, and it'll read the appropriate translated data from the
loaded data files. A StringBuffer is returned.

To allow variables embedded in text, StringBuffer::operator<< is used
in a similar way to in cout, storing the variable in the StringBuffer and
returning the StringBuffer again.
StringBuffer::operator Str() does the final insertion of variables into
the translated phrase, either grabbing the final result from a cache or
doing all the variable-to-string conversions.


The strings read from disk are each stored as a TranslatedString,
containing several TSComponents, which are either static strings or
default-formatted variables or functions.

TSComponentFunction is the most complex of the TSComponents, containing a
list of ScriptValues -- these allow numbers, strings and variables to be
passed as parameters into a JS function.


StringBuffer::operator<< stores BufferVariable*s in the StringBuffer.
These provide access to a hash of the variable (for caching), and can
be converted into a string (for display).


StrImW is used in various places, just as a more efficient alternative
to std::wstring.


*/

#include "precompiled.h"

#include "Interface.h"
#include "CLocale.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "i18n"

using namespace I18n;

struct JSContext;

CLocale_interface* I18n::NewLocale(JSContext* cx, JSObject* scope)
{
	try
	{
		return new CLocale(cx, scope);
	}
	catch (PSERROR_I18n& e)
	{
		LOG(ERROR, LOG_CATEGORY, "Error creating locale object ('%ls')", GetErrorString(e.code));
		return NULL;
	}
}
