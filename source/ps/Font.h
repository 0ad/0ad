#ifndef _FONT_H
#define _FONT_H

#include "ps/Singleton.h"
#include "ps/CStr.h"

#include "lib/res/handle.h"

#include <map>
#include <string>

/*

To use CFont:

CFont font("name");
font.Bind();
glwprintf(L"Hello world");

*/

// MFC has nicked all the good names :-(
#define CFont PS_CFont

class CFont
{
public:
	CFont(const char* name);
	~CFont();

	void Bind();
	int GetLineSpacing();
	int GetHeight();
	int GetCharacterWidth(wchar_t c);
	void CalculateStringSize(const CStrW& string, int& w, int& h);

private:
	Handle h;
};


#endif // _FONT_H
