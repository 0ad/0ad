#include "precompiled.h"

#include "TranslatedString.h"
#include "TSComponent.h"

using namespace I18n;

TranslatedString::~TranslatedString()
{
	for (std::vector<const TSComponent*>::iterator it = Parts.begin(); it != Parts.end(); ++it)
		delete *it;
}
