//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 02/21/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_dds.c
//
// Description: Reads from a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------


//
//
// Note:  Almost all this code is from nVidia's DDS-loading example at
//	http://www.nvidia.com/view.asp?IO=dxtc_decompression_code
//	and from the specs at
//	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dx8_c/hh/dx8_c/graphics_using_0j03.asp
//	and
//	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dx8_c/directx_cpp/Graphics/ProgrammersGuide/Appendix/DDSFileFormat/ovwDDSFileFormat.asp
//	However, some not really valid .dds files are also read, for example
//	Volume Textures without the COMPLEX bit set, so the specs aren't taken
//	too strictly while reading.


#include "il_internal.h"
#ifndef IL_NO_DDS
#include "il_dds.h"


// Global variables
DDSHEAD	Head;			// Image header
ILubyte	*CompData;		// Compressed data
ILuint	CompSize;		// Compressed size
ILuint	CompLineSize;		// Compressed line size
ILuint	CompFormat;		// Compressed format
ILimage	*Image;
ILint	Width, Height, Depth;
ILuint	BlockSize;

ILuint CubemapDirections[CUBEMAP_SIDES] = {
	DDS_CUBEMAP_POSITIVEX,
	DDS_CUBEMAP_NEGATIVEX,
	DDS_CUBEMAP_POSITIVEY,
	DDS_CUBEMAP_NEGATIVEY,
	DDS_CUBEMAP_POSITIVEZ,
	DDS_CUBEMAP_NEGATIVEZ
};


//! Checks if the file specified in FileName is a valid .dds file.
ILboolean ilIsValidDds(const ILstring FileName)
{
	ILHANDLE	DdsFile;
	ILboolean	bDds = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("dds"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bDds;
	}

	DdsFile = iopenr(FileName);
	if (DdsFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDds;
	}

	bDds = ilIsValidDdsF(DdsFile);
	icloser(DdsFile);

	return bDds;
}


//! Checks if the ILHANDLE contains a valid .dds file at the current position.
ILboolean ilIsValidDdsF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidDds();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid .dds lump.
ILboolean ilIsValidDdsL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidDds();
}


// Internal function used to get the .dds header from the current file.
ILboolean iGetDdsHead(DDSHEAD *Header)
{
	if (iread(Header, sizeof(DDSHEAD), 1) != 1)
		return IL_FALSE;

	Int(&Header->Size1);
	Int(&Header->Flags1);
	Int(&Header->Height);
	Int(&Header->Width);
	Int(&Header->LinearSize);
	Int(&Header->Depth);
	Int(&Header->MipMapCount);
	Int(&Header->AlphaBitDepth);
	Int(&Header->Size2);
	Int(&Header->Flags2);
	Int(&Header->FourCC);
	Int(&Header->RGBBitCount);
	Int(&Header->RBitMask);
	Int(&Header->GBitMask);
	Int(&Header->BBitMask);
	Int(&Header->RGBAlphaBitMask);
	Int(&Header->ddsCaps1);
	Int(&Header->ddsCaps2);
	Int(&Header->ddsCaps3);
	Int(&Header->ddsCaps4);
	Int(&Header->TextureStage);

	if (Head.Depth == 0)
		Head.Depth = 1;

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidDds()
{
	ILboolean	IsValid;
	DDSHEAD		Head;

	iGetDdsHead(&Head);
	iseek(-(ILint)sizeof(DDSHEAD), IL_SEEK_CUR);  // Go ahead and restore to previous state

	IsValid = iCheckDds(&Head);

	return IsValid;
}


// Internal function used to check if the HEADER is a valid .dds header.
ILboolean iCheckDds(DDSHEAD *Head)
{
	if (strnicmp(Head->Signature, "DDS ", 4))
		return IL_FALSE;
	if (Head->Size1 != 124)
		return IL_FALSE;
	if (Head->Size2 != 32)
		return IL_FALSE;
	if (Head->Width == 0 || Head->Height == 0)
		return IL_FALSE;
	return IL_TRUE;
}


//! Reads a .dds file
ILboolean ilLoadDds(const ILstring FileName)
{
	ILHANDLE	DdsFile;
	ILboolean	bDds = IL_FALSE;

	DdsFile = iopenr(FileName);
	if (DdsFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bDds;
	}

	bDds = ilLoadDdsF(DdsFile);
	icloser(DdsFile);

	return bDds;
}


//! Reads an already-opened .dds file
ILboolean ilLoadDdsF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadDdsInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a .dds
ILboolean ilLoadDdsL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadDdsInternal();
}


ILboolean iLoadDdsCubemapInternal()
{
	ILuint	i;
	ILubyte	Bpp;
	ILimage *startImage;

	CompData = NULL;

	if (CompFormat == PF_RGB)
		Bpp = 3;
	else
		Bpp = 4;

	startImage = Image;
	// run through cube map possibilities
	for (i = 0; i < CUBEMAP_SIDES; i++) {
		// reset each time
		Width = Head.Width;
		Height = Head.Height;
		Depth = Head.Depth;
		if (Head.ddsCaps2 & CubemapDirections[i]) {
			if (i != 0) {
				Image->Next = ilNewImage(Width, Height, Depth, Bpp, 1);
				if (Image->Next == NULL)
					return IL_FALSE;

				Image = Image->Next;
				startImage->NumNext++;
				ilBindImage(ilGetCurName());  // Set to parent image first.
				ilActiveImage(i);
			}

			Image->CubeFlags = CubemapDirections[i];

			if (!ReadData())
				return IL_FALSE;

			if (!AllocImage()) {
				if (CompData) {
					ifree(CompData);
					CompData = NULL;
				}
				return IL_FALSE;
			}

			if (!Decompress()) {
				if (CompData) {
					ifree(CompData);
					CompData = NULL;
				}
				return IL_FALSE;
			}

			if (!ReadMipmaps()) {
				if (CompData) {
					ifree(CompData);
					CompData = NULL;
				}
				return IL_FALSE;
			}
		}
	}

	if (CompData) {
		ifree(CompData);
		CompData = NULL;
	}

	ilBindImage(ilGetCurName());  // Set to parent image first.
	return ilFixImage();
}


ILboolean iLoadDdsInternal()
{
	CompData = NULL;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	if (!iGetDdsHead(&Head)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	if(!iCheckDds(&Head)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	DecodePixelFormat();
	if (CompFormat == PF_UNKNOWN) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	// Microsoft bug, they're not following their own documentation.
	if (!(Head.Flags1 & (DDS_LINEARSIZE | DDS_PITCH))) {
		Head.Flags1 |= DDS_LINEARSIZE;
		Head.LinearSize = BlockSize;
	}

	Image = iCurImage;
	if (Head.ddsCaps1 & DDS_COMPLEX) {
		if (Head.ddsCaps2 & DDS_CUBEMAP) {
			if (!iLoadDdsCubemapInternal())
				return IL_FALSE;
			return IL_TRUE;
		}
	}
	Width = Head.Width;
	Height = Head.Height;
	Depth = Head.Depth;

	AdjustVolumeTexture(&Head);

	if (!ReadData())
		return IL_FALSE;
	if (!AllocImage()) {
		if (CompData)
			ifree(CompData);
		return IL_FALSE;
	}
	if (!Decompress()) {
		if (CompData)
			ifree(CompData);
		return IL_FALSE;
	}

	if (!ReadMipmaps()) {
		if (CompData)
			ifree(CompData);
		return IL_FALSE;
	}

	if (CompData) {
		ifree(CompData);
		CompData = NULL;
	}

	ilBindImage(ilGetCurName());  // Set to parent image first.
	return ilFixImage();
}


ILvoid DecodePixelFormat()
{
	if (Head.Flags2 & DDS_FOURCC) {
		BlockSize = ((Head.Width + 3)/4) * ((Head.Height + 3)/4) * ((Head.Depth + 3)/4);
		switch (Head.FourCC)
		{
			case IL_MAKEFOURCC('D','X','T','1'):
				CompFormat = PF_DXT1;
				BlockSize *= 8;
				break;

			case IL_MAKEFOURCC('D','X','T','2'):
				CompFormat = PF_DXT2;
				BlockSize *= 16;
				break;

			case IL_MAKEFOURCC('D','X','T','3'):
				CompFormat = PF_DXT3;
				BlockSize *= 16;
				break;

			case IL_MAKEFOURCC('D','X','T','4'):
				CompFormat = PF_DXT4;
				BlockSize *= 16;
				break;

			case IL_MAKEFOURCC('D','X','T','5'):
				CompFormat = PF_DXT5;
				BlockSize *= 16;
				break;

			default:
				CompFormat = PF_UNKNOWN;
				BlockSize *= 16;
				break;
		}
	} else {
	// This dds texture isn't compressed so write out ARGB format
		if (Head.Flags2 & DDS_ALPHAPIXELS) {
			CompFormat = PF_ARGB;
		} else {
			CompFormat = PF_RGB;
		}
		BlockSize = (Head.Width * Head.Height * Head.Depth * (Head.RGBBitCount >> 3));
	}
	return;
}


// The few volume textures that I have don't have consistent LinearSize
//	entries, even thouh the DDS_LINEARSIZE flag is set.
ILvoid AdjustVolumeTexture(DDSHEAD *Head)
{
	if (Head->Depth <= 1)
		return;

	// All volume textures I've seem so far didn't have the DDS_COMPLEX flag set,
	// even though this is normally required. But because noone does set it,
	// also read images without it (TODO: check file size for 3d texture?)
	if (/*!(Head->ddsCaps1 & DDS_COMPLEX) ||*/ !(Head->ddsCaps2 & DDS_VOLUME)) {
		Head->Depth = 1;
		Depth = 1;
	}

	switch (CompFormat)
	{
		case PF_ARGB:
		case PF_RGB:
			Head->LinearSize = IL_MAX(1,Head->Width) * IL_MAX(1,Head->Height) *
				(Head->RGBBitCount / 8);
			break;
	
		case PF_DXT1:
			Head->LinearSize = IL_MAX(1,Head->Width/4) * IL_MAX(1,Head->Height/4) * 8;
			break;

		case PF_DXT2:
		case PF_DXT3:
		case PF_DXT4:
		case PF_DXT5:
			Head->LinearSize = IL_MAX(1,Head->Width/4) * IL_MAX(1,Head->Height/4) * 16;
			break;
	}

	Head->Flags1 |= DDS_LINEARSIZE;
	Head->LinearSize *= Head->Depth;

	return;
}


// Reads the compressed data
ILboolean ReadData()
{
	ILuint	Bps;
	ILint	y, z;
	ILubyte	*Temp;
	ILuint	Bpp;

	if (CompFormat == PF_RGB)
		Bpp = 3;
	else
		Bpp = 4;

	if (CompData) {
		ifree(CompData);
		CompData = NULL;
	}

	if (Head.Flags1 & DDS_LINEARSIZE) {
		//Head.LinearSize = Head.LinearSize * Depth;

		CompData = (ILubyte*)ialloc(Head.LinearSize);
		if (CompData == NULL) {
			return IL_FALSE;
		}

		if (iread(CompData, 1, Head.LinearSize) != (ILuint)Head.LinearSize) {
			ifree(CompData);
			CompData = NULL;
			return IL_FALSE;
		}
	}
	else {
		Bps = Width * Head.RGBBitCount / 8;
		CompSize = Bps * Height * Depth;
		CompLineSize = Bps;

		CompData = (ILubyte*)ialloc(CompSize);
		if (CompData == NULL) {
			return IL_FALSE;
		}

		Temp = CompData;
		for (z = 0; z < Depth; z++) {
			for (y = 0; y < Height; y++) {
				if (iread(Temp, 1, Bps) != Bps) {
					ifree(CompData);
					CompData = NULL;
					return IL_FALSE;
				}
				Temp += Bps;
			}
		}
	}

	return IL_TRUE;
}


ILboolean AllocImage()
{
	switch (CompFormat)
	{
		case PF_RGB:
			if (!ilTexImage(Width, Height, Depth, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			break;
		case PF_ARGB:
			if (!ilTexImage(Width, Height, Depth, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			break;
		default:
			if (!ilTexImage(Width, Height, Depth, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL))
				return IL_FALSE;
			if (ilGetInteger(IL_KEEP_DXTC_DATA) == IL_TRUE) {
				iCurImage->DxtcData = (ILubyte*)ialloc(Head.LinearSize);
				if (iCurImage->DxtcData == NULL)
					return IL_FALSE;
				iCurImage->DxtcFormat = CompFormat - PF_DXT1 + IL_DXT1;
				iCurImage->DxtcSize = Head.LinearSize;
				memcpy(iCurImage->DxtcData, CompData, iCurImage->DxtcSize);
			}
			break;
	}

	Image->Origin = IL_ORIGIN_UPPER_LEFT;
	
	return IL_TRUE;
}


ILboolean Decompress()
{
	switch (CompFormat)
	{
		case PF_ARGB:
		case PF_RGB:
			return DecompressARGB();

		case PF_DXT1:
			return DecompressDXT1();

		case PF_DXT2:
			return DecompressDXT2();

		case PF_DXT3:
			return DecompressDXT3();	

		case PF_DXT4:
			return DecompressDXT4();

		case PF_DXT5:
			return DecompressDXT5();		

		case PF_UNKNOWN:
			return IL_FALSE;
	}

	return IL_FALSE;
}


ILboolean ReadMipmaps()
{
	ILuint	i, CompFactor=0;
	ILubyte	Bpp;
	ILimage	*StartImage, *TempImage;
	ILuint	LastLinear;
	ILuint	minW, minH;

	if (CompFormat == PF_RGB)
		Bpp = 3;
	else
		Bpp = 4;

	if (Head.Flags1 & DDS_LINEARSIZE) {
		CompFactor = (Width * Height * Depth * Bpp) / Head.LinearSize;
	}

	StartImage = Image;

	if (!(Head.Flags1 & DDS_MIPMAPCOUNT) || Head.MipMapCount == 0) {
		//some .dds-files have their mipmap flag set,
		//but a mipmapcount of 0. Because mipMapCount is an uint, 0 - 1 gives
		//overflow - don't let this happen:
		Head.MipMapCount = 1;
	}

	LastLinear = Head.LinearSize;
	for (i = 0; i < Head.MipMapCount - 1; i++) {
		Depth = Depth / 2;
		Width = Width / 2;
		Height = Height / 2;

		if (Depth == 0) 
			Depth = 1;
		if (Width == 0) 
			Width = 1;
		if (Height == 0) 
			Height = 1;

		//TODO: mipmaps don't keep DXT data???
		Image->Next = ilNewImage(Width, Height, Depth, Bpp, 1);
		if (Image->Next == NULL)
			goto mip_fail;
		Image = Image->Next;
		Image->Origin = IL_ORIGIN_UPPER_LEFT;

		if (Head.Flags1 & DDS_LINEARSIZE) {
			minW = Width;
			minH = Height;
			if ((CompFormat != PF_RGB) && (CompFormat != PF_ARGB)) {
				minW = IL_MAX(4, Width);
				minH = IL_MAX(4, Height);
				Head.LinearSize = (minW * minH * Depth * Bpp) / CompFactor;
			}
			else {
				Head.LinearSize = Width * Height * Depth * (Head.RGBBitCount >> 3);
			}
		}
		else {
			Head.LinearSize >>= 1;
		}

		if (!ReadData())
			goto mip_fail;
		if (!Decompress())
			goto mip_fail;
	}

	Head.LinearSize = LastLinear;
	StartImage->Mipmaps = StartImage->Next;
	StartImage->Next = NULL;
	StartImage->NumMips = Head.MipMapCount - 1;
	Image = StartImage;

	return IL_TRUE;

mip_fail:
	Image = StartImage;
	StartImage = StartImage->Next;
	while (StartImage) {
		TempImage = StartImage;
		StartImage = StartImage->Next;
		ilCloseImage(TempImage);
	}
	Image->Next = NULL;
	return IL_FALSE;
}


ILboolean DecompressDXT1()
{
	int			x, y, z, i, j, k, Select;
	ILubyte		*Temp;
	Color565	*color_0, *color_1;
	Color8888	colours[4], *col;
	ILuint		bitmask, Offset;


	Temp = CompData;
	for (z = 0; z < Depth; z++) {
		for (y = 0; y < Height; y += 4) {
			for (x = 0; x < Width; x += 4) {

				color_0 = ((Color565*)Temp);
				color_1 = ((Color565*)(Temp+2));
				bitmask = ((ILuint*)Temp)[1];
				Temp += 8;

				// "To convert 5 or 6 bits to eight, the upper bits are copied to the lower bits."
				// (in NVIDIA hardware, apparently - see http://sourceforge.net/mailarchive/forum.php?thread_id=6550301&forum_id=6188)
				colours[0].b = (color_0->nBlue  << 3) | (color_0->nBlue  >> 2);
				colours[0].g = (color_0->nGreen << 2) | (color_0->nGreen >> 4);
				colours[0].r = (color_0->nRed   << 3) | (color_0->nRed   >> 2);
				colours[0].a = 0xFF;

				colours[1].b = (color_1->nBlue  << 3) | (color_1->nBlue  >> 2);
				colours[1].g = (color_1->nGreen << 2) | (color_1->nGreen >> 4);
				colours[1].r = (color_1->nRed   << 3) | (color_1->nRed   >> 2);
				colours[1].a = 0xFF;


				if (*((ILushort*)color_0) > *((ILushort*)color_1)) {
					// Four-color block: derive the other two colors.    
					// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
					// These 2-bit codes correspond to the 2-bit fields 
					// stored in the 64-bit block.
					colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
					colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
					colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
					colours[2].a = 0xFF;

					colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
					colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
					colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
					colours[3].a = 0xFF;
				}    
				else { 
					// Three-color block: derive the other color.
					// 00 = color_0,  01 = color_1,  10 = color_2,
					// 11 = transparent.
					// These 2-bit codes correspond to the 2-bit fields 
					// stored in the 64-bit block. 
					colours[2].b = (colours[0].b + colours[1].b) / 2;
					colours[2].g = (colours[0].g + colours[1].g) / 2;
					colours[2].r = (colours[0].r + colours[1].r) / 2;
					colours[2].a = 0xFF;

					colours[3].b = 0x00;
					colours[3].g = 0x00;
					colours[3].r = 0x00;
					colours[3].a = 0x00;
				}

				for (j = 0, k = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {

						Select = (bitmask & (0x03 << k*2)) >> k*2;
						col = &colours[Select];

						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp;
							Image->Data[Offset + 0] = col->r;
							Image->Data[Offset + 1] = col->g;
							Image->Data[Offset + 2] = col->b;
							Image->Data[Offset + 3] = col->a;
						}
					}
				}
			}
		}
	}

	return IL_TRUE;
}


ILboolean DecompressDXT2()
{
	// Can do color & alpha same as dxt3, but color is pre-multiplied 
	//   so the result will be wrong unless corrected. 
	if (!DecompressDXT3())
		return IL_FALSE;
	CorrectPreMult();

	return IL_TRUE;
}


ILboolean DecompressDXT3()
{
	int			x, y, z, i, j, k, Select;
	ILubyte		*Temp;
	Color565	*color_0, *color_1;
	Color8888	colours[4], *col;
	ILuint		bitmask, Offset;
	ILushort	word;
	DXTAlphaBlockExplicit *alpha;


	Temp = CompData;
	for (z = 0; z < Depth; z++) {
		for (y = 0; y < Height; y += 4) {
			for (x = 0; x < Width; x += 4) {
				alpha = (DXTAlphaBlockExplicit*)Temp;
				Temp += 8;
				color_0 = ((Color565*)Temp);
				color_1 = ((Color565*)(Temp+2));
				bitmask = ((ILuint*)Temp)[1];
				Temp += 8;

				// "To convert 5 or 6 bits to eight, the upper bits are copied to the lower bits."
				// (in NVIDIA hardware, apparently - see http://sourceforge.net/mailarchive/forum.php?thread_id=6550301&forum_id=6188)
				colours[0].b = (color_0->nBlue  << 3) | (color_0->nBlue  >> 2);
				colours[0].g = (color_0->nGreen << 2) | (color_0->nGreen >> 4);
				colours[0].r = (color_0->nRed   << 3) | (color_0->nRed   >> 2);
				colours[0].a = 0xFF;

				colours[1].b = (color_1->nBlue  << 3) | (color_1->nBlue  >> 2);
				colours[1].g = (color_1->nGreen << 2) | (color_1->nGreen >> 4);
				colours[1].r = (color_1->nRed   << 3) | (color_1->nRed   >> 2);
				colours[1].a = 0xFF;

				// Four-color block: derive the other two colors.    
				// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
				// These 2-bit codes correspond to the 2-bit fields 
				// stored in the 64-bit block.
				colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
				colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
				colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
				colours[2].a = 0xFF;

				colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
				colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
				colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
				colours[3].a = 0xFF;

				k = 0;
				for (j = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {

						Select = (bitmask & (0x03 << k*2)) >> k*2;
						col = &colours[Select];

						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp;
							Image->Data[Offset + 0] = col->r;
							Image->Data[Offset + 1] = col->g;
							Image->Data[Offset + 2] = col->b;
						}
					}
				}

				for (j = 0; j < 4; j++) {
					word = alpha->row[j];
					for (i = 0; i < 4; i++) {
						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp + 3;
							Image->Data[Offset] = word & 0x0F;
							Image->Data[Offset] = Image->Data[Offset] | (Image->Data[Offset] << 4);
						}
						word >>= 4;
					}
				}

			}
		}
	}

	return IL_TRUE;
}


ILboolean DecompressDXT4()
{
	// Can do color & alpha same as dxt5, but color is pre-multiplied 
	//   so the result will be wrong unless corrected. 
	if (!DecompressDXT5())
		return IL_FALSE;
	CorrectPreMult();

	return IL_FALSE;
}


ILboolean DecompressDXT5()
{
	int			x, y, z, i, j, k, Select;
	ILubyte		*Temp;
	Color565	*color_0, *color_1;
	Color8888	colours[4], *col;
	ILuint		bitmask, Offset;
	ILubyte		alphas[8], *alphamask;
	ILuint		bits;

	Temp = CompData;
	for (z = 0; z < Depth; z++) {
		for (y = 0; y < Height; y += 4) {
			for (x = 0; x < Width; x += 4) {
				if (y >= Height || x >= Width)
					break;
				alphas[0] = Temp[0];
				alphas[1] = Temp[1];
				alphamask = Temp + 2;
				Temp += 8;
				color_0 = ((Color565*)Temp);
				color_1 = ((Color565*)(Temp+2));
				bitmask = ((ILuint*)Temp)[1];
				Temp += 8;

				// "To convert 5 or 6 bits to eight, the upper bits are copied to the lower bits."
				// (in NVIDIA hardware, apparently - see http://sourceforge.net/mailarchive/forum.php?thread_id=6550301&forum_id=6188)
				colours[0].b = (color_0->nBlue  << 3) | (color_0->nBlue  >> 2);
				colours[0].g = (color_0->nGreen << 2) | (color_0->nGreen >> 4);
				colours[0].r = (color_0->nRed   << 3) | (color_0->nRed   >> 2);
				colours[0].a = 0xFF;

				colours[1].b = (color_1->nBlue  << 3) | (color_1->nBlue  >> 2);
				colours[1].g = (color_1->nGreen << 2) | (color_1->nGreen >> 4);
				colours[1].r = (color_1->nRed   << 3) | (color_1->nRed   >> 2);
				colours[1].a = 0xFF;

				// Four-color block: derive the other two colors.    
				// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
				// These 2-bit codes correspond to the 2-bit fields 
				// stored in the 64-bit block.
				colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
				colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
				colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
				colours[2].a = 0xFF;

				colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
				colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
				colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
				colours[3].a = 0xFF;

				k = 0;
				for (j = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {

						Select = (bitmask & (0x03 << k*2)) >> k*2;
						col = &colours[Select];

						// only put pixels out < width or height
						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp;
							Image->Data[Offset + 0] = col->r;
							Image->Data[Offset + 1] = col->g;
							Image->Data[Offset + 2] = col->b;
						}
					}
				}

				// 8-alpha or 6-alpha block?    
				if (alphas[0] > alphas[1]) {    
					// 8-alpha block:  derive the other six alphas.    
					// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
					alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;	// bit code 010
					alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;	// bit code 011
					alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;	// bit code 100
					alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;	// bit code 101
					alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;	// bit code 110
					alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;	// bit code 111  
				}    
				else {  
					// 6-alpha block.    
					// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
					alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;	// Bit code 010
					alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;	// Bit code 011
					alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;	// Bit code 100
					alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;	// Bit code 101
					alphas[6] = 0x00;										// Bit code 110
					alphas[7] = 0xFF;										// Bit code 111
				}

				// Note: Have to separate the next two loops,
				//	it operates on a 6-byte system.

				// First three bytes
				bits = *((ILint*)alphamask);
				for (j = 0; j < 2; j++) {
					for (i = 0; i < 4; i++) {
						// only put pixels out < width or height
						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp + 3;
							Image->Data[Offset] = alphas[bits & 0x07];
						}
						bits >>= 3;
					}
				}

				// Last three bytes
				bits = *((ILint*)&alphamask[3]);
				for (j = 2; j < 4; j++) {
					for (i = 0; i < 4; i++) {
						// only put pixels out < width or height
						if (((x + i) < Width) && ((y + j) < Height)) {
							Offset = z * Image->SizeOfPlane + (y + j) * Image->Bps + (x + i) * Image->Bpp + 3;
							Image->Data[Offset] = alphas[bits & 0x07];
						}
						bits >>= 3;
					}
				}
			}
		}
	}

	return IL_TRUE;
}


ILvoid CorrectPreMult()
{
	ILuint i;

	for (i = 0; i < Image->SizeOfData; i += 4) {
		if (Image->Data[i+3] != 0) {  // Cannot divide by 0.
			Image->Data[i]   = (ILubyte)(((ILuint)Image->Data[i]   << 8) / Image->Data[i+3]);
			Image->Data[i+1] = (ILubyte)(((ILuint)Image->Data[i+1] << 8) / Image->Data[i+3]);
			Image->Data[i+2] = (ILubyte)(((ILuint)Image->Data[i+2] << 8) / Image->Data[i+3]);
		}
	}

	return;
}


ILboolean DecompressARGB()
{
	ILuint	i, ReadI, RedL, RedR, GreenL, GreenR, BlueL, BlueR, AlphaL, AlphaR;
	ILubyte	*Temp;

	GetBitsFromMask(Head.RBitMask, &RedL, &RedR);
	GetBitsFromMask(Head.GBitMask, &GreenL, &GreenR);
	GetBitsFromMask(Head.BBitMask, &BlueL, &BlueR);
	GetBitsFromMask(Head.RGBAlphaBitMask, &AlphaL, &AlphaR);
	Temp = CompData;

	for (i = 0; i < Image->SizeOfData; i += Image->Bpp) {
		ReadI = *((ILuint*)Temp);
		Temp += (Head.RGBBitCount / 8);

		Image->Data[i]   = ((ReadI & Head.RBitMask) >> RedR) << RedL;
		Image->Data[i+1] = ((ReadI & Head.GBitMask) >> GreenR) << GreenL;
		Image->Data[i+2] = ((ReadI & Head.BBitMask) >> BlueR) << BlueL;

		if (Image->Bpp == 4) {
			Image->Data[i+3] = ((ReadI & Head.RGBAlphaBitMask) >> AlphaR) << AlphaL;
			if (AlphaL >= 7) {
				Image->Data[i+3] = Image->Data[i+3] ? 0xFF : 0x00;
			}
			else if (AlphaL >= 4) {
				Image->Data[i+3] = Image->Data[i+3] | (Image->Data[i+3] >> 4);
			}
		}
	}

	return IL_TRUE;
}


// @TODO:  Look at using the BSF/BSR operands for inline ASM here.
ILvoid GetBitsFromMask(ILuint Mask, ILuint *ShiftLeft, ILuint *ShiftRight)
{
	ILuint Temp, i;

	if (Mask == 0) {
		*ShiftLeft = *ShiftRight = 0;
		return;
	}

	Temp = Mask;
	for (i = 0; i < 32; i++, Temp >>= 1) {
		if (Temp & 1)
			break;
	}
	*ShiftRight = i;

	// Temp is preserved, so use it again:
	for (i = 0; i < 8; i++, Temp >>= 1) {
		if (!(Temp & 1))
			break;
	}
	*ShiftLeft = 8 - i;

	return;
}


#endif//IL_NO_DDS
