#include "NPFont.h"

NPFont::NPFont() 
{
}

NPFont::~NPFont() 
{
}

NPFont* NPFont::create(const char* name)
{
	// try and open up the file
	FILE* fp=fopen(name,"rb");
	if (!fp) return 0;

	// create a new font
	NPFont* font=new NPFont;

	// read text metrics
	if (fread(&font->_metrics,sizeof(font->_metrics),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}

	// read characters
	if (fread(font->_chars,sizeof(CharData)*128,1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}

	// read number of rows, cols of characters
	if (fread(&font->_numRows,sizeof(font->_numRows),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}
	if (fread(&font->_numCols,sizeof(font->_numCols),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}

	// read texture dimensions
	if (fread(&font->_texwidth,sizeof(font->_texwidth),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}
	if (fread(&font->_texheight,sizeof(font->_texheight),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}

	// read texture name
	unsigned int namelen;
	if (fread(&namelen,sizeof(unsigned int),1,fp)!=1) {
		fclose(fp);
		delete font;
		return 0;
	}
	CStr texname("gui/fonts/");
	for (uint i=0;i<namelen;i++) {
		char c;
		if (fread(&c,sizeof(char),1,fp)!=1) {
			fclose(fp);
			delete font;
			return 0;
		}
		texname+=c;
	}
	font->_texture.SetName((const char*) texname);

	// store font name
	font->_name=name;

	// return created font
	return font;
}

// GetOutputStringSize: return the rendered size of given font
void NPFont::GetOutputStringSize(const char* str,int& sx,int& sy)
{
	sx=0;
	sy=_metrics._height;

	int i=0;
	while (str && str[i]!='\0') {
		const NPFont::CharData& cdata=chardata(str[i]);
		const int* cw=&cdata._widthA;

		if (cw[0]>0) sx+=cw[0];
		sx+=cw[1]+1;
		if (cw[2]>0) sx+=cw[2];

		i++;
	}
}

