//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 01/29/2002 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_psd.c
//
// Description: Reads from a PhotoShop (.psd) file.
//
//-----------------------------------------------------------------------------

// Information about the .psd format was taken from Adobe's PhotoShop SDK at
//  http://partners.adobe.com/asn/developer/gapsdk/PhotoshopSDK.html
//  Information about the Packbits compression scheme was found at
//	http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf

#include "il_internal.h"
#ifndef IL_NO_PSD
#include "il_psd.h"


//! Checks if the file specified in FileName is a valid Psd file.
ILboolean ilIsValidPsd(const ILstring FileName)
{
	ILHANDLE	PsdFile;
	ILboolean	bPsd = IL_FALSE;

	if (!iCheckExtension(FileName, IL_TEXT("psd")) &&
		!iCheckExtension(FileName, IL_TEXT("pdd"))) {
		ilSetError(IL_INVALID_EXTENSION);
		return bPsd;
	}

	PsdFile = iopenr(FileName);
	if (PsdFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPsd;
	}

	bPsd = ilIsValidPsdF(PsdFile);
	icloser(PsdFile);

	return bPsd;
}


//! Checks if the ILHANDLE contains a valid Psd file at the current position.
ILboolean ilIsValidPsdF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iIsValidPsd();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Checks if Lump is a valid Psd lump.
ILboolean ilIsValidPsdL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iIsValidPsd();
}


// Internal function used to get the Psd header from the current file.
ILboolean iGetPsdHead(PSDHEAD *Header)
{
	if (iread(Header, sizeof(PSDHEAD), 1) != 1)
		return IL_FALSE;

	BigUShort(&Header->Version);
	BigUShort(&Header->Channels);
	BigUInt(&Header->Height);
	BigUInt(&Header->Width);
	BigUShort(&Header->Depth);
	BigUShort(&Header->Mode);

	return IL_TRUE;
}


// Internal function to get the header and check it.
ILboolean iIsValidPsd()
{
	PSDHEAD	Head;

	iGetPsdHead(&Head);
	iseek(-(ILint)sizeof(PSDHEAD), IL_SEEK_CUR);

	return iCheckPsd(&Head);
}


// Internal function used to check if the HEADER is a valid Psd header.
ILboolean iCheckPsd(PSDHEAD *Header)
{
	ILuint i;

	if (strncmp(Header->Signature, "8BPS", 4))
		return IL_FALSE;
	if (Header->Version != 1)
		return IL_FALSE;
	for (i = 0; i < 6; i++) {
		if (Header->Reserved[i] != 0)
			return IL_FALSE;
	}
	if (Header->Channels < 1 || Header->Channels > 24)
		return IL_FALSE;
	if (Header->Height < 1 || Header->Width < 1)
		return IL_FALSE;
	if (Header->Depth != 1 && Header->Depth != 8 && Header->Depth != 16)
		return IL_FALSE;

	return IL_TRUE;
}


//! Reads a Psd file
ILboolean ilLoadPsd(const ILstring FileName)
{
	ILHANDLE	PsdFile;
	ILboolean	bPsd = IL_FALSE;

	PsdFile = iopenr(FileName);
	if (PsdFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPsd;
	}

	bPsd = ilLoadPsdF(PsdFile);
	icloser(PsdFile);

	return bPsd;
}


//! Reads an already-opened Psd file
ILboolean ilLoadPsdF(ILHANDLE File)
{
	ILuint		FirstPos;
	ILboolean	bRet;

	iSetInputFile(File);
	FirstPos = itell();
	bRet = iLoadPsdInternal();
	iseek(FirstPos, IL_SEEK_SET);

	return bRet;
}


//! Reads from a memory "lump" that contains a Psd
ILboolean ilLoadPsdL(ILvoid *Lump, ILuint Size)
{
	iSetInputLump(Lump, Size);
	return iLoadPsdInternal();
}


// Internal function used to load the Psd.
ILboolean iLoadPsdInternal()
{
	PSDHEAD	Header;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	iGetPsdHead(&Header);
	if (!iCheckPsd(&Header)) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}

	if (!ReadPsd(&Header))
		return IL_FALSE;
	iCurImage->Origin = IL_ORIGIN_UPPER_LEFT;

	ilFixImage();

	return IL_TRUE;
}


ILboolean ReadPsd(PSDHEAD *Head)
{
	switch (Head->Mode)
	{
		case 1:  // Greyscale
			return ReadGrey(Head);
		case 2:  // Indexed
			return ReadIndexed(Head);
		case 3:  // RGB
			return ReadRGB(Head);
		case 4:  // CMYK
			return ReadCMYK(Head);
	}

	ilSetError(IL_FORMAT_NOT_SUPPORTED);
	return IL_FALSE;
}


ILboolean ReadGrey(PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Type;
	ILubyte		*Resources = NULL;

	ColorMode = GetBigUInt();  // Skip over the 'color mode data section'
	iseek(ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt();  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (iread(Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt();
	iseek(MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort();

	ChannelNum = Head->Channels;
	Head->Channels = 1;  // Temporary to read only one channel...some greyscale .psd files have 2.
	if (Head->Channels != 1) {
		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}

	if (!ilTexImage(Head->Width, Head->Height, 1, 1, IL_LUMINANCE, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(Head, iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;
	if (!ParseResources(ResourceSize, Resources))
		goto cleanup_error;
	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}


ILboolean ReadIndexed(PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo, i, j, NumEnt;
	ILushort	Compressed;
	ILubyte		*Palette = NULL, *Resources = NULL;

	ColorMode = GetBigUInt();  // Skip over the 'color mode data section'
	if (ColorMode % 3 != 0) {
		ilSetError(IL_INVALID_FILE_HEADER);
		return IL_FALSE;
	}
	Palette = (ILubyte*)ialloc(ColorMode);
	if (Palette == NULL)
		return IL_FALSE;
	if (iread(Palette, 1, ColorMode) != ColorMode)
		goto cleanup_error;

	ResourceSize = GetBigUInt();  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (iread(Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt();
	if (ieof())
		goto cleanup_error;
	iseek(MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort();
	if (ieof())
		goto cleanup_error;

	if (Head->Channels != 1 || Head->Depth != 8) {
		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		goto cleanup_error;
	}
	ChannelNum = Head->Channels;

	if (!ilTexImage(Head->Width, Head->Height, 1, 1, IL_COLOUR_INDEX, IL_UNSIGNED_BYTE, NULL))
		goto cleanup_error;

	iCurImage->Pal.Palette = (ILubyte*)ialloc(ColorMode);
	if (iCurImage->Pal.Palette == NULL) {
		goto cleanup_error;
	}
	iCurImage->Pal.PalSize = ColorMode;
	iCurImage->Pal.PalType = IL_PAL_RGB24;

	NumEnt = iCurImage->Pal.PalSize / 3;
	for (i = 0, j = 0; i < iCurImage->Pal.PalSize; i += 3, j++) {
		iCurImage->Pal.Palette[i  ] = Palette[j];
		iCurImage->Pal.Palette[i+1] = Palette[j+NumEnt];
		iCurImage->Pal.Palette[i+2] = Palette[j+NumEnt*2];
	}
	ifree(Palette);
	Palette = NULL;

	if (!PsdGetData(Head, iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;

	ParseResources(ResourceSize, Resources);
	ifree(Resources);
	Resources = NULL;

	return IL_TRUE;

cleanup_error:
	ifree(Palette);
	ifree(Resources);

	return IL_FALSE;
}


ILboolean ReadRGB(PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte		*Resources = NULL;

	ColorMode = GetBigUInt();  // Skip over the 'color mode data section'
	iseek(ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt();  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL)
		return IL_FALSE;
	if (iread(Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt();
	iseek(MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort();

	ChannelNum = Head->Channels;
	switch (Head->Channels)
	{
		case 3:
			Format = IL_RGB;
			break;
		case 4:
			Format = IL_RGBA;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	if (!ilTexImage(Head->Width, Head->Height, 1, (ILubyte)Head->Channels, Format, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(Head, iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;
	if (!ParseResources(ResourceSize, Resources))
		goto cleanup_error;
	ifree(Resources);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	return IL_FALSE;
}


ILboolean ReadCMYK(PSDHEAD *Head)
{
	ILuint		ColorMode, ResourceSize, MiscInfo, Size, i, j;
	ILushort	Compressed;
	ILenum		Format, Type;
	ILubyte		*Resources = NULL, *KChannel = NULL;

	ColorMode = GetBigUInt();  // Skip over the 'color mode data section'
	iseek(ColorMode, IL_SEEK_CUR);

	ResourceSize = GetBigUInt();  // Read the 'image resources section'
	Resources = (ILubyte*)ialloc(ResourceSize);
	if (Resources == NULL) {
		return IL_FALSE;
	}
	if (iread(Resources, 1, ResourceSize) != ResourceSize)
		goto cleanup_error;

	MiscInfo = GetBigUInt();
	iseek(MiscInfo, IL_SEEK_CUR);

	Compressed = GetBigUShort();

	switch (Head->Channels)
	{
		case 4:
			Format = IL_RGB;
			ChannelNum = 4;
			Head->Channels = 3;
			break;
		case 5:
			Format = IL_RGBA;
			ChannelNum = 5;
			Head->Channels = 4;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	switch (Head->Depth)
	{
		case 8:
			Type = IL_UNSIGNED_BYTE;
			break;
		case 16:
			Type = IL_UNSIGNED_SHORT;
			break;
		default:
			ilSetError(IL_FORMAT_NOT_SUPPORTED);
			return IL_FALSE;
	}
	if (!ilTexImage(Head->Width, Head->Height, 1, (ILubyte)Head->Channels, Format, Type, NULL))
		goto cleanup_error;
	if (!PsdGetData(Head, iCurImage->Data, (ILboolean)Compressed))
		goto cleanup_error;

	Size = iCurImage->Bpc * iCurImage->Width * iCurImage->Height;
	KChannel = (ILubyte*)ialloc(Size);
	if (KChannel == NULL)
		goto cleanup_error;
	if (!GetSingleChannel(Head, KChannel, (ILboolean)Compressed))
		goto cleanup_error;

	if (Format == IL_RGB) {
		for (i = 0, j = 0; i < iCurImage->SizeOfData; i += 3, j++) {
			iCurImage->Data[i  ] = (iCurImage->Data[i  ] * KChannel[j]) >> 8;
			iCurImage->Data[i+1] = (iCurImage->Data[i+1] * KChannel[j]) >> 8;
			iCurImage->Data[i+2] = (iCurImage->Data[i+2] * KChannel[j]) >> 8;
		}
	}
	else {  // IL_RGBA
		// The KChannel array really holds the alpha channel on this one.
		for (i = 0, j = 0; i < iCurImage->SizeOfData; i += 4, j++) {
			iCurImage->Data[i  ] = (iCurImage->Data[i  ] * iCurImage->Data[i+3]) >> 8;
			iCurImage->Data[i+1] = (iCurImage->Data[i+1] * iCurImage->Data[i+3]) >> 8;
			iCurImage->Data[i+2] = (iCurImage->Data[i+2] * iCurImage->Data[i+3]) >> 8;
			iCurImage->Data[i+3] = KChannel[j];  // Swap 'K' with alpha channel.
		}
	}

	if (!ParseResources(ResourceSize, Resources))
		goto cleanup_error;

	ifree(Resources);
	ifree(KChannel);

	return IL_TRUE;

cleanup_error:
	ifree(Resources);
	ifree(KChannel);
	return IL_FALSE;
}


ILuint *GetCompChanLen(PSDHEAD *Head)
{
	ILushort	*RleTable;
	ILuint		*ChanLen, c, i, j;

	RleTable = (ILushort*)ialloc(Head->Height * ChannelNum * sizeof(ILushort));
	ChanLen = (ILuint*)ialloc(ChannelNum * sizeof(ILuint));
	if (RleTable == NULL || ChanLen == NULL) {
		return NULL;
	}

	if (iread(RleTable, sizeof(ILushort), Head->Height * ChannelNum) != Head->Height * ChannelNum) {
		ifree(RleTable);
		ifree(ChanLen);
		return NULL;
	}
#ifdef __LITTLE_ENDIAN__
	for (i = 0; i < Head->Height * ChannelNum; i++) {
		RleTable[i] = SwapShort(RleTable[i]);
	}
#endif

	imemclear(ChanLen, ChannelNum * sizeof(ILuint));
	for (c = 0; c < ChannelNum; c++) {
		j = c * Head->Height;
		for (i = 0; i < Head->Height; i++) {
			ChanLen[c] += RleTable[i + j];
		}
	}

	ifree(RleTable);

	return ChanLen;
}


ILboolean PsdGetData(PSDHEAD *Head, ILvoid *Buffer, ILboolean Compressed)
{
	ILuint		c, x, y, i, Size;
	ILubyte		*Channel = NULL;
	ILushort	*ShortPtr;
	ILbyte		HeadByte;
	ILint		Run;
	ILuint		*ChanLen = NULL;
	ILboolean	PreCache = IL_FALSE;

	Channel = (ILubyte*)ialloc(Head->Width * Head->Height * iCurImage->Bpc);
	if (Channel == NULL) {
		return IL_FALSE;
	}
	ShortPtr = (ILushort*)Channel;

	// @TODO: Add support for this in, though I have yet to run across a .psd
	//	file that uses this.
	if (Compressed && iCurImage->Type == IL_UNSIGNED_SHORT) {
		ilSetError(IL_FORMAT_NOT_SUPPORTED);
		return IL_FALSE;
	}

	if (!Compressed) {
		if (iCurImage->Bpc == 1) {
			for (c = 0; c < Head->Channels; c++) {
				i = 0;
				if (iread(Channel, Head->Width * Head->Height, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				for (y = 0; y < Head->Height * iCurImage->Bps; y += iCurImage->Bps) {
					for (x = 0; x < iCurImage->Bps; x += iCurImage->Bpp, i++) {
						iCurImage->Data[y + x + c] = Channel[i];
					}
				}
			}
		}
		else {  // iCurImage->Bpc == 2
			for (c = 0; c < Head->Channels; c++) {
				i = 0;
				if (iread(Channel, Head->Width * Head->Height * 2, 1) != 1) {
					ifree(Channel);
					return IL_FALSE;
				}
				iCurImage->Bps /= 2;
				for (y = 0; y < Head->Height * iCurImage->Bps; y += iCurImage->Bps) {
					for (x = 0; x < iCurImage->Bps; x += iCurImage->Bpp, i++) {
						((ILushort*)iCurImage->Data)[y + x + c] = ShortPtr[i];
					}
				}
				iCurImage->Bps *= 2;
			}
		}
	}
	else {
		ChanLen = GetCompChanLen(Head);
		if (iGetHint(IL_MEM_SPEED_HINT) == IL_FASTEST)
			PreCache = IL_TRUE;

		Size = Head->Width * Head->Height;
		for (c = 0; c < Head->Channels; c++) {
			if (PreCache)
				iPreCache(ChanLen[c]);
			for (i = 0; i < Size; ) {
				HeadByte = igetc();

				if (HeadByte >= 0) {  //  && HeadByte <= 127
					if (i + HeadByte > Size)
						goto file_corrupt;
					if (iread(Channel + i, HeadByte + 1, 1) != 1)
						goto file_read_error;

					i += HeadByte + 1;
				}
				if (HeadByte >= -127 && HeadByte <= -1) {
					Run = igetc();
					if (Run == IL_EOF)
						goto file_read_error;
					if (i + (-HeadByte + 1) > Size)
						goto file_corrupt;

					memset(Channel + i, Run, -HeadByte + 1);
					i += -HeadByte + 1;
				}
				if (HeadByte == -128)
				{ }  // Noop
			}
			if (PreCache)
				iUnCache();

			i = 0;
			for (y = 0; y < Head->Height * iCurImage->Bps; y += iCurImage->Bps) {
				for (x = 0; x < iCurImage->Bps; x += iCurImage->Bpp, i++) {
					iCurImage->Data[y + x + c] = Channel[i];
				}
			}
		}

		ifree(ChanLen);
	}

	ifree(Channel);

	return IL_TRUE;

file_corrupt:
	ifree(ChanLen);
	ifree(Channel);
	if (PreCache)
		iUnCache();
	ilSetError(IL_ILLEGAL_FILE_VALUE);
	return IL_FALSE;

file_read_error:
	ifree(ChanLen);
	ifree(Channel);
	if (PreCache)
		iUnCache();
	return IL_FALSE;
}


ILboolean ParseResources(ILuint ResourceSize, ILubyte *Resources)
{
	ILushort	ID;
	ILubyte		NameLen;
	ILuint		Size;

	if (Resources == NULL) {
		ilSetError(IL_INTERNAL_ERROR);
		return IL_FALSE;
	}

	while (ResourceSize) {
		if (strncmp("8BIM", (ILbyte*)Resources, 4)) {
			//return IL_FALSE;
			return IL_TRUE;  // 05-30-2002: May not necessarily mean corrupt data...
		}
		Resources += 4;

		ID = *((ILushort*)Resources);
		BigUShort(&ID);
		Resources += 2;

		NameLen = *Resources++;
		// NameLen + the byte it occupies must be padded to an even number, so NameLen must be odd.
		NameLen = NameLen + (NameLen & 1 ? 0 : 1);
		Resources += NameLen;

		// Get the resource data size.
		Size = *((ILuint*)Resources);
		BigUInt(&Size);
		Resources += 4;

		ResourceSize -= (4 + 2 + 1 + NameLen);

		switch (ID)
		{
			case 0x040F:  // ICC Profile
				iCurImage->Profile = (ILubyte*)ialloc(Size);
				if (iCurImage->Profile == NULL) {
					return IL_FALSE;
				}
				memcpy(iCurImage->Profile, Resources, Size);
				iCurImage->ProfileSize = Size;
				break;

			default:
				break;
		}

		if (Size & 1)  // Must be an even number.
			Size++;
		ResourceSize -= Size;
		Resources += Size;
	}

	return IL_TRUE;
}


ILboolean GetSingleChannel(PSDHEAD *Head, ILubyte *Buffer, ILboolean Compressed)
{
	ILuint		i;
	ILushort	*ShortPtr;
	ILbyte		HeadByte;
	ILint		Run;

	ShortPtr = (ILushort*)Buffer;

	if (!Compressed) {
		if (iCurImage->Bpc == 1) {
			if (iread(Buffer, Head->Width * Head->Height, 1) != 1)
				return IL_FALSE;
		}
		else {  // iCurImage->Bpc == 2
			if (iread(Buffer, Head->Width * Head->Height * 2, 1) != 1)
				return IL_FALSE;
		}
	}
	else {
		for (i = 0; i < Head->Width * Head->Height; ) {
			HeadByte = igetc();

			if (HeadByte >= 0) {  //  && HeadByte <= 127
				if (iread(Buffer + i, HeadByte + 1, 1) != 1)
					return IL_FALSE;
				i += HeadByte + 1;
			}
			if (HeadByte >= -127 && HeadByte <= -1) {
				Run = igetc();
				if (Run == IL_EOF)
					return IL_FALSE;
				memset(Buffer + i, Run, -HeadByte + 1);
				i += -HeadByte + 1;
			}
			if (HeadByte == -128)
			{ }  // Noop
		}
	}

	return IL_TRUE;
}



//! Writes a Psd file
ILboolean ilSavePsd(const ILstring FileName)
{
	ILHANDLE	PsdFile;
	ILboolean	bPsd = IL_FALSE;

	if (ilGetBoolean(IL_FILE_MODE) == IL_FALSE) {
		if (iFileExists(FileName)) {
			ilSetError(IL_FILE_ALREADY_EXISTS);
			return IL_FALSE;
		}
	}

	PsdFile = iopenw(FileName);
	if (PsdFile == NULL) {
		ilSetError(IL_COULD_NOT_OPEN_FILE);
		return bPsd;
	}

	bPsd = ilSavePsdF(PsdFile);
	iclosew(PsdFile);

	return bPsd;
}


//! Writes a Psd to an already-opened file
ILboolean ilSavePsdF(ILHANDLE File)
{
	iSetOutputFile(File);
	return iSavePsdInternal();
}


//! Writes a Psd to a memory "lump"
ILboolean ilSavePsdL(ILvoid *Lump, ILuint Size)
{
	iSetOutputLump(Lump, Size);
	return iSavePsdInternal();
}


// Internal function used to save the Psd.
ILboolean iSavePsdInternal()
{
	ILubyte		*Signature = "8BPS";
	ILimage		*TempImage;
	ILpal		*TempPal;
	ILuint		c, i;
	ILubyte		*TempData;
	ILushort	*ShortPtr;
	ILenum		Format, Type;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Format = iCurImage->Format;
	Type = iCurImage->Type;

	// All of these comprise the actual signature.
	iwrite(Signature, 1, 4);
	SaveBigShort(1);
	SaveBigInt(0);
	SaveBigShort(0);

	SaveBigShort(iCurImage->Bpp);
	SaveBigInt(iCurImage->Height);
	SaveBigInt(iCurImage->Width);
	if (iCurImage->Bpc > 2)
		Type = IL_UNSIGNED_SHORT;

	if (iCurImage->Format == IL_BGR)
		Format = IL_RGB;
	else if (iCurImage->Format == IL_BGRA)
		Format = IL_RGBA;

	if (Format != iCurImage->Format || Type != iCurImage->Type) {
		TempImage = iConvertImage(iCurImage, Format, Type);
		if (TempImage == NULL)
			return IL_FALSE;
	}
	else {
		TempImage = iCurImage;
	}
	SaveBigShort((ILushort)(TempImage->Bpc * 8));

	// @TODO:  Put the other formats here.
	switch (TempImage->Format)
	{
		case IL_COLOUR_INDEX:
			SaveBigShort(2);
			break;
		case IL_LUMINANCE:
			SaveBigShort(1);
			break;
		case IL_RGB:
		case IL_RGBA:
			SaveBigShort(3);
			break;
		default:
			ilSetError(IL_INTERNAL_ERROR);
			return IL_FALSE;
	}

	if (TempImage->Format == IL_COLOUR_INDEX) {
		// @TODO: We're currently making a potentially fatal assumption that
		//	iConvertImage was not called if the format is IL_COLOUR_INDEX.
		TempPal = iConvertPal(&TempImage->Pal, IL_PAL_RGB24);
		if (TempPal == NULL)
			return IL_FALSE;
		SaveBigInt(768);

		// Have to save the palette in a planar format.
		for (c = 0; c < 3; c++) {
			for (i = c; i < TempPal->PalSize; i += 3) {
				iputc(TempPal->Palette[i]);
			}
		}

		ifree(TempPal->Palette);
	}
	else {
		SaveBigInt(0);  // No colour mode data.
	}

	SaveBigInt(0);  // No image resources.
	SaveBigInt(0);  // No layer information.
	SaveBigShort(0);  // Raw data, no compression.

	// @TODO:  Add RLE compression.

	if (TempImage->Origin == IL_ORIGIN_LOWER_LEFT) {
		TempData = iGetFlipped(TempImage);
		if (TempData == NULL) {
			ilCloseImage(TempImage);
			return IL_FALSE;
		}
	}
	else {
		TempData = TempImage->Data;
	}

	if (TempImage->Bpc == 1) {
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				iputc(TempData[i]);
			}
		}
	}
	else {  // TempImage->Bpc == 2
		ShortPtr = (ILushort*)TempData;
		TempImage->SizeOfPlane /= 2;
		for (c = 0; c < TempImage->Bpp; c++) {
			for (i = c; i < TempImage->SizeOfPlane; i += TempImage->Bpp) {
				SaveBigUShort(ShortPtr[i]);
			}
		}
		TempImage->SizeOfPlane *= 2;
	}

	if (TempData != TempImage->Data)
		ifree(TempData);

	if (TempImage != iCurImage)
		ilCloseImage(TempImage);


	return IL_TRUE;
}


#endif//IL_NO_PSD
