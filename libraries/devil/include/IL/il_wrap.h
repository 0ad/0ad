#ifndef WRAPPER_H
#define WRAPPER_H

/*#include <il/il.h>
#include <il/ilu.h>*/
#include <il/ilut.h>  // Probably only have to #include this one

#ifdef _MSC_VER
	#ifndef _IL_WRAP_BUILD_LIB
		#pragma comment(lib, "il_wrap.lib")
	#endif
#endif

class ilImage
{
public:
				ilImage();
				ilImage(char *);
				ilImage(const ilImage &);
	virtual		~ilImage();

	ILboolean	Load(char *);
	ILboolean	Load(char *, ILenum);
	ILboolean	Save(char *);
	ILboolean	Save(char *, ILenum);


	// ImageLib functions
	ILboolean	ActiveImage(ILuint);
	ILboolean	ActiveLayer(ILuint);
	ILboolean	ActiveMipmap(ILuint);
	ILboolean	Clear(ILvoid);
	ILvoid		ClearColour(ILubyte, ILubyte, ILubyte, ILubyte);
	ILboolean	Convert(ILenum);
	ILboolean	Copy(ILuint);
	ILboolean	Default(ILvoid);
	ILboolean	Flip(ILvoid);
	ILboolean	SwapColours(ILvoid);
	ILboolean	Resize(ILuint, ILuint, ILuint);
	ILboolean	TexImage(ILuint, ILuint, ILuint, ILubyte, ILenum, ILenum, ILvoid*);

	
	// Image handling
	ILvoid		Bind(ILvoid) const;
	ILvoid		Bind(ILuint);
	ILvoid		Close(ILvoid) { this->Delete(); }
	ILvoid		Delete(ILvoid);
	ILvoid		iGenBind();


	// Image characteristics
	ILuint		Width(ILvoid);
	ILuint		Height(ILvoid);
	ILuint		Depth(ILvoid);
	ILubyte		Bpp(ILvoid);
	ILubyte		Bitpp(ILvoid);
	ILenum		PaletteType(ILvoid);
	ILenum		Format(ILvoid);
	ILenum		Type(ILvoid);
	ILuint		NumImages(ILvoid);
	ILuint		NumMipmaps(ILvoid);
	ILuint		GetId(ILvoid) const;
    ILenum      GetOrigin(ILvoid);
	ILubyte		*GetData(ILvoid);
	ILubyte		*GetPalette(ILvoid);


	// Rendering
	ILuint		BindImage(ILvoid);
	ILuint		BindImage(ILenum);


	// Operators
	ilImage&	operator = (ILuint);
	ilImage&	operator = (const ilImage &);


protected:
	ILuint		Id;

private:
	ILvoid		iStartUp();


};


class ilFilters
{
public:
	static ILboolean	Alienify(ilImage &);
	static ILboolean	BlurAvg(ilImage &, ILuint Iter);
	static ILboolean	BlurGaussian(ilImage &, ILuint Iter);
	static ILboolean	Contrast(ilImage &, ILfloat Contrast);
	static ILboolean	EdgeDetectE(ilImage &);
	static ILboolean	EdgeDetectP(ilImage &);
	static ILboolean	EdgeDetectS(ilImage &);
	static ILboolean	Emboss(ilImage &);
	static ILboolean	Gamma(ilImage &, ILfloat Gamma);
	static ILboolean	Negative(ilImage &);
	static ILboolean	Noisify(ilImage &, ILubyte Factor);
	static ILboolean	Pixelize(ilImage &, ILuint PixSize);
	static ILboolean	Saturate(ilImage &, ILfloat Saturation);
	static ILboolean	Saturate(ilImage &, ILfloat r, ILfloat g, ILfloat b, ILfloat Saturation);
	static ILboolean	ScaleColours(ilImage &, ILfloat r, ILfloat g, ILfloat b);
	static ILboolean	Sharpen(ilImage &, ILfloat Factor, ILuint Iter);
};


#ifdef ILUT_USE_OPENGL
class ilOgl
{
public:
	static ILvoid		Init(ILvoid);
	static GLuint		BindTex(ilImage &);
	static ILboolean	Upload(ilImage &, ILuint);
	static GLuint		Mipmap(ilImage &);
	static ILboolean	Screen(ILvoid);
	static ILboolean	Screenie(ILvoid);
};
#endif//ILUT_USE_OPENGL


#ifdef ILUT_USE_ALLEGRO
class ilAlleg
{
public:
	static ILvoid	Init(ILvoid);
	static BITMAP	*Convert(ilImage &);
};
#endif//ILUT_USE_ALLEGRO


#ifdef ILUT_USE_WIN32
class ilWin32
{
public:
	static ILvoid		Init(ILvoid);
	static HBITMAP		Convert(ilImage &);
	static ILboolean	GetClipboard(ilImage &);
	static ILvoid		GetInfo(ilImage &, BITMAPINFO *Info);
	static ILubyte		*GetPadData(ilImage &);
	static HPALETTE		GetPal(ilImage &);
	static ILboolean	GetResource(ilImage &, HINSTANCE hInst, ILint ID, char *ResourceType);
	static ILboolean	GetResource(ilImage &, HINSTANCE hInst, ILint ID, char *ResourceType, ILenum Type);
	static ILboolean	SetClipboard(ilImage &);
};
#endif//ILUT_USE_WIN32


class ilValidate
{
public:
	static ILboolean	Valid(ILenum, char *);
	static ILboolean	Valid(ILenum, FILE *);
	static ILboolean	Valid(ILenum, ILvoid *, ILuint);

protected:

private:

};


class ilState
{
public:
	static ILboolean		Disable(ILenum);
	static ILboolean		Enable(ILenum);
	static ILvoid			Get(ILenum, ILboolean &);
	static ILvoid			Get(ILenum, ILint &);
	static ILboolean		GetBool(ILenum);
	static ILint			GetInt(ILenum);
	static const char		*GetString(ILenum);
	static ILboolean		IsDisabled(ILenum);
	static ILboolean		IsEnabled(ILenum);
	static ILboolean		Origin(ILenum);
	static ILvoid			Pop(ILvoid);
	static ILvoid			Push(ILuint);


protected:

private:

};


class ilError
{
public:
	static ILvoid		Check(ILvoid (*Callback)(const char*));
	static ILvoid		Check(ILvoid (*Callback)(ILenum));
	static ILenum		Get(ILvoid);
	static const char	*String(ILvoid);
	static const char	*String(ILenum);

protected:

private:

};


#endif//WRAPPER_H