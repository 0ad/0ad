#include "NPFontManager.h"
#include "NPFont.h"

#include <algorithm>

// the sole instance of the NPFontManager
NPFontManager* NPFontManager::_instance=0;

///////////////////////////////////////////////////////////////////////////////
// instance: return the sole font manager instance
NPFontManager& NPFontManager::instance()
{
	if (!_instance) {
		_instance=new NPFontManager;
	}
	return *_instance;
}

///////////////////////////////////////////////////////////////////////////////
// NPFontManager constructor (hidden); access available only through instance()
NPFontManager::NPFontManager()
{
}

///////////////////////////////////////////////////////////////////////////////
// NPFontManager destructor 
NPFontManager::~NPFontManager()
{
	// delete all collected fonts
	for (uint i=0;i<_fonts.size();++i) {
		delete _fonts[i];
	}
}

///////////////////////////////////////////////////////////////////////////////
// release this font manager
void NPFontManager::release()
{
	if (_instance) {
		delete _instance;
		_instance=0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// add a font; return the created font, or 0 if not found
NPFont* NPFontManager::add(const char* name)
{
	// try and find font first
	NPFont* font=find(name);
	if (font) {
		// ok - already got this font, return it
		return font;
	}

	// try and create font
	font=NPFont::create(name);
	if (font) {
		// success - add to list
		_fonts.push_back(font);
		return font;
	} else {
		// mark as bad? just return failure for the moment
		return 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// remove a font
bool NPFontManager::remove(const char* name)
{
	// try and find font first
	NPFont* font=find(name);
	if (!font) {
		// font not present ..
		return false;
	} else {
		typedef std::vector<NPFont*>::iterator Iter;
		Iter iter=std::find(_fonts.begin(),_fonts.end(),font);
		assert(iter != _fonts.end());
		_fonts.erase(iter);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// find a font (return 0 if not found)
NPFont* NPFontManager::find(const char* name)
{
	// scan through font list checking names
	for (uint i=0;i<_fonts.size();++i) {
		NPFont* font=_fonts[i];
		if (strcmp(name,font->name())==0) {
			return font;
		}
	}

	// got this far, font not found
	return 0;

}
