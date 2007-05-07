#ifndef INCLUDED_FONT
#define INCLUDED_FONT

#include "lib/res/handle.h"

class CStrW;

/*

To use CFont:

CFont font("name");
font.Bind();
glwprintf(L"Hello world");

*/

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


#endif // INCLUDED_FONT
