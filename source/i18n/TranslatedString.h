/*

Simple storage for translated phrases, made up of lots of TSComponents
(strings / variables / functions)

*/

#ifndef I18N_TSTRING_H
#define I18N_TSTRING_H

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

#endif // I18N_TSTRING_H
