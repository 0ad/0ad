#include "precompiled.h"

#include "Font.h"

#include "ps/ConfigDB.h"
#include "lib/res/unifont.h"
#include "lib/res/h_mgr.h"	// h_filename, needed for refcnt hack below

#include "ps/CLogger.h"
#define LOG_CATEGORY "graphics"

#include <map>
#include <string>

const char* DefaultFont = "verdana16";

CFont::CFont(const char* name)
{
	// TODO perhaps: cache the resultant filename (or Handle) for each
	// font name; but it's nice to allow run-time alteration of the fonts

	std::string fontFilename;

	CStr fontName = "font."; fontName += name;

	// See if the config value can be loaded
	CConfigValue* fontFilenameVar = g_ConfigDB.GetValue(CFG_SYSTEM, fontName);
	if (fontFilenameVar && fontFilenameVar->GetString(fontFilename))
	{
		h = unifont_load(fontFilename.c_str());
	}
	else
	{
		// Not found in the config file -- try it as a simple filename
		h = unifont_load(name);

		// Found it
		if (h > 0)
			return;

		// Not found as a font -- give up and use the default.
		LOG_ONCE(ERROR, LOG_CATEGORY, "Failed to find font '%s'", name);
		h = unifont_load(DefaultFont);
		// Assume this worked
	}
}

CFont::~CFont()
{
	unifont_unload(h);
}

// Copy constructor
CFont::CFont(const CFont& font)
{
	// Reload the font, to maintain the ref count
	h = unifont_load( h_filename(font.h) );
}

void CFont::Bind()
{
	unifont_bind(h);
}

int CFont::GetLineSpacing()
{
	return unifont_linespacing(h);
}

void CFont::CalculateStringSize(const CStrW& string, int& width, int& height)
{
	unifont_stringsize(h, (const wchar_t*)string, width, height);
}
