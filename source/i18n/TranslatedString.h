/*

Simple storage for translated phrases, made up of lots of TSComponents
(strings / variables / functions)

*/

#ifndef INCLUDED_I18N_TRANSLATEDSTRING
#define INCLUDED_I18N_TRANSLATEDSTRING

#include <vector>

namespace I18n
{
	class TSComponent;

	class TranslatedString
	{
	public:
		std::vector<const TSComponent*> Parts;
		unsigned char VarCount;

		~TranslatedString();
	};

}

#endif // INCLUDED_I18N_TRANSLATEDSTRING
