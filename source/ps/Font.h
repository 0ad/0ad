#ifndef _FONT_H
#define _FONT_H

#include "ps/Singleton.h"
#include "ps/CStr.h"

#include "lib/res/h_mgr.h"

#include <map>
#include <string>

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

	CFont(const CFont&);

	void Bind();
	int GetLineSpacing();
	void CalculateStringSize(const CStrW& string, int& w, int& h);

private:
	Handle h;
};


#endif // _FONT_H
