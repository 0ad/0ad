#ifndef _FONTMANAGER_H
#define _FONTMANAGER_H

// necessary includes
#include <vector>

// necessary declaration
class NPFont;

class NPFontManager
{
public:
	// accessor; return the font manager
	static NPFontManager& instance();
	// release this font manager
	static void release();

	// destructor 
	~NPFontManager();

	// add a font; return the created font, or 0 if not found
	NPFont* add(const char* name);
	// remove a font
	bool remove(const char* name);
	// find a font (return 0 if not found)
	NPFont* find(const char* name);

private:
	// hidden constructor; access available only through instance()
	NPFontManager();

	// list of recorded fonts
	std::vector<NPFont*> _fonts;

	// the sole instance of the FontManager
	static NPFontManager* _instance;
};

#endif
