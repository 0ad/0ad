//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 06/13/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_fastconv.c
//
// Description: Converts between several image formats
//
//-----------------------------------------------------------------------------


#include "il_internal.h"


ILboolean iFastConvert(ILenum DestFormat)
{
	ILuint		SizeOfData, i=0;
	ILubyte		*BytePtr = iCurImage->Data, TempByte=0;
	ILushort	*ShortPtr = (ILushort*)iCurImage->Data, TempShort=0;
	ILuint		*IntPtr = (ILuint*)iCurImage->Data, TempInt=0;
	ILfloat		*FloatPtr = (ILfloat*)iCurImage->Data, TempFloat=0;
	ILdouble	*DblPtr = (ILdouble*)iCurImage->Data, TempDbl = 0;

	// We assume iCurImage is valid, since this is called from ilConvertImage.

	switch (DestFormat)
	{
		case IL_RGB:
		case IL_BGR:
			if (iCurImage->Format != IL_RGB && iCurImage->Format != IL_BGR)
				return IL_FALSE;

			switch (iCurImage->Type)
			{
				case IL_BYTE:
				case IL_UNSIGNED_BYTE:
					SizeOfData = iCurImage->SizeOfData / 3;
					#ifdef USE_WIN32_ASM
						__asm
						{
							mov ebx, BytePtr
							mov ecx, SizeOfData
							L1:
								mov al,[ebx+0]
								xchg al,[ebx+2]
								mov [ebx+0],al
								add ebx,3
								loop L1
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempByte = BytePtr[0];
							BytePtr[0] = BytePtr[2];
							BytePtr[2] = TempByte;
							BytePtr += 3;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_SHORT:
				case IL_UNSIGNED_SHORT:
					SizeOfData = iCurImage->SizeOfData / 6;  // 3*2
					#ifdef USE_WIN32_ASM
						__asm
						{
							mov ebx, ShortPtr
							mov ecx, SizeOfData
							L2:
								mov ax,[ebx+0]
								xchg ax,[ebx+4]
								mov [ebx+0],ax
								add ebx,6
								loop L2
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempShort = ShortPtr[0];
							ShortPtr[0] = ShortPtr[2];
							ShortPtr[2] = TempShort;
							ShortPtr += 3;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_INT:
				case IL_UNSIGNED_INT:
					SizeOfData = iCurImage->SizeOfData / 12;  // 3*4
					#ifdef USE_WIN32_ASM
						__asm
						{
							mov ebx, IntPtr
							mov ecx, SizeOfData
							L3:
								mov eax,[ebx+0]
								xchg eax,[ebx+8]
								mov [ebx+0],ax
								add ebx,12
								loop L3
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempInt = IntPtr[0];
							IntPtr[0] = IntPtr[2];
							IntPtr[2] = TempInt;
							IntPtr += 3;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_FLOAT:
					SizeOfData = iCurImage->SizeOfData / 12;  // 3*4
					for (i = 0; i < SizeOfData; i++) {
						TempFloat = FloatPtr[0];
						FloatPtr[0] = FloatPtr[2];
						FloatPtr[2] = TempFloat;
						FloatPtr += 3;
					}
					return IL_TRUE;

				case IL_DOUBLE:
					SizeOfData = iCurImage->SizeOfData / 24;  // 3*8
					for (i = 0; i < SizeOfData; i++) {
						TempDbl = DblPtr[0];
						DblPtr[0] = DblPtr[2];
						DblPtr[2] = TempDbl;
						DblPtr += 3;
					}
					return IL_TRUE;
			}
			break;

		case IL_RGBA:
		case IL_BGRA:
			if (iCurImage->Format != IL_RGBA && iCurImage->Format != IL_BGRA)
				return IL_FALSE;

			switch (iCurImage->Type)
			{
				case IL_BYTE:
				case IL_UNSIGNED_BYTE:
					SizeOfData = iCurImage->SizeOfData / 4;
					#ifdef USE_WIN32_ASM
						__asm
						{
							/*mov ebx, BytePtr
							mov ecx, SizeOfData
							L4:
								mov al,[ebx+0]
								xchg al,[ebx+2]
								mov [ebx+0],al
								add ebx,4
								loop L4*/

							mov ebx, BytePtr
							mov ecx, SizeOfData
							L4:
								mov eax,[ebx]
								bswap eax
								ror eax,8
								mov [ebx], eax
								add ebx,4
								loop L4
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempByte = BytePtr[0];
							BytePtr[0] = BytePtr[2];
							BytePtr[2] = TempByte;
							BytePtr += 4;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_SHORT:
				case IL_UNSIGNED_SHORT:
					SizeOfData = iCurImage->SizeOfData / 8;  // 4*2
					#ifdef USE_WIN32_ASM
						__asm
						{
							mov ebx, ShortPtr
							mov ecx, SizeOfData
							L5:
								mov ax,[ebx+0]
								xchg ax,[ebx+4]
								mov [ebx+0],ax
								add ebx,8
								loop L5
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempShort = ShortPtr[0];
							ShortPtr[0] = ShortPtr[2];
							ShortPtr[2] = TempShort;
							ShortPtr += 4;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_INT:
				case IL_UNSIGNED_INT:
					SizeOfData = iCurImage->SizeOfData / 16;  // 4*4
					#ifdef USE_WIN32_ASM
						__asm
						{
							mov ebx, IntPtr
							mov ecx, SizeOfData
							L6:
								mov eax,[ebx+0]
								xchg eax,[ebx+8]
								mov [ebx+0],ax
								add ebx,16
								loop L6
						}
					#else
						for (i = 0; i < SizeOfData; i++) {
							TempInt = IntPtr[0];
							IntPtr[0] = IntPtr[2];
							IntPtr[2] = TempInt;
							IntPtr += 4;
						}
					#endif//USE_WIN32_ASM
					return IL_TRUE;

				case IL_FLOAT:
					SizeOfData = iCurImage->SizeOfData / 16;  // 4*4
					for (i = 0; i < SizeOfData; i++) {
						TempFloat = FloatPtr[0];
						FloatPtr[0] = FloatPtr[2];
						FloatPtr[2] = TempFloat;
						FloatPtr += 4;
					}
					return IL_TRUE;

				case IL_DOUBLE:
					SizeOfData = iCurImage->SizeOfData / 32;  // 4*8
					for (i = 0; i < SizeOfData; i++) {
						TempDbl = DblPtr[0];
						DblPtr[0] = DblPtr[2];
						DblPtr[2] = TempDbl;
						DblPtr += 4;
					}
					return IL_TRUE;
			}
	}


	return IL_FALSE;
}

