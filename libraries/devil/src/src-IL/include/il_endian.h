//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 01/29/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/include/il_endian.h
//
// Description: Handles Endian-ness
//
//-----------------------------------------------------------------------------

#ifndef IL_ENDIAN_H
#define IL_ENDIAN_H

#include "il_internal.h"

#ifdef __BIG_ENDIAN__
	#define Short(s) _SwapShort(s)
	#define UShort(s) _SwapUShort(s)
	#define Int(i) _SwapInt(i)
	#define UInt(i) _SwapUInt(i)
	#define Float(f) _SwapFloat(f)
	#define Double(d) _SwapDouble(d)
 
	#define BigShort(s)  
	#define BigUShort(s)  
	#define BigInt(i)  
	#define BigUInt(i)  
	#define BigFloat(f)  
	#define BigDouble(d)  
#else
	#undef __LITTLE_ENDIAN__  // Not sure if it's defined by any compiler...
	#define __LITTLE_ENDIAN__
	#define Short(s)  
	#define UShort(s)  
	#define Int(i)  
	#define UInt(i)  
	#define Float(f)  
	#define Double(d)  

	#define BigUShort(s) _SwapUShort(s)
	#define BigUInt(i) _SwapUInt(i)
	#define BigInt(i) _SwapInt(i)
	#define BigUShort(s) _SwapUShort(s)
	#define BigUInt(i) _SwapUInt(i)
	#define BigFloat(f) _SwapFloat(f)
	#define BigDouble(d) _SwapDouble(d)
#endif


ILvoid	_SwapUShort(ILushort *s);
ILvoid	_SwapShort(ILshort *s);
ILvoid	_SwapUInt(ILuint *i);
ILvoid	_SwapInt(ILint *i);
ILvoid	_SwapFloat(ILfloat *f);
ILvoid	_SwapDouble(ILdouble *d);


// Only seem to work on unsigned values properly...
#define SwapShort(s) ((s>>8) | (s<<8))
#define SwapInt(i) ((i>>24) | ((i>>8) & 0xff00) | ((i<<8) & 0xff0000) | (i<<24))
//#define SwapFloat(f) ((f>>24) | ((f>>8) & 0xff00) | ((f<<8) & 0xff0000) | (f<<24))
//#define SwapDouble(d) ((f>>56) | ((f>>40) & 0xff00) | ((f>>24) & 0xff0000) |
//	((f>>8) & 0xff000000) | ((f<<8) & 0xff00000000) | ((f<<24) & 0xff0000000000) | ((f<<40) & 0xff000000000000) | (f<<56))


ILushort	GetLittleUShort(ILvoid);
ILshort		GetLittleShort(ILvoid);
ILuint		GetLittleUInt(ILvoid);
ILint		GetLittleInt(ILvoid);
ILfloat		GetLittleFloat(ILvoid);
ILdouble	GetLittleDouble(ILvoid);
ILushort	GetBigUShort(ILvoid);
ILshort		GetBigShort(ILvoid);
ILuint		GetBigUInt(ILvoid);
ILint		GetBigInt(ILvoid);
ILfloat		GetBigFloat(ILvoid);
ILdouble	GetBigDouble(ILvoid);

ILubyte		SaveLittleUShort(ILushort _s);
ILubyte		SaveLittleShort(ILshort _s);
ILubyte		SaveLittleUInt(ILuint _i);
ILubyte		SaveLittleInt(ILint _i);
ILubyte		SaveLittleFloat(ILfloat _f);
ILubyte		SaveLittleDouble(ILdouble _d);
ILubyte		SaveBigUShort(ILushort _s);
ILubyte		SaveBigShort(ILshort _s);
ILubyte		SaveBigUInt(ILuint _i);
ILubyte		SaveBigInt(ILint _i);
ILubyte		SaveBigFloat(ILfloat _f);
ILubyte		SaveBigDouble(ILdouble _d);

ILvoid		EndianSwapData(void *_Image);

#endif//ENDIAN_H
