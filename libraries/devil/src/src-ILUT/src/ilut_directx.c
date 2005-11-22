//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 12/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_directx.c
//
// Description: DirectX functions for textures
//
//-----------------------------------------------------------------------------

#include "ilut_internal.h"
#ifdef ILUT_USE_DIRECTX8

#include <d3d8.h>
#include <d3dx8tex.h>
#pragma comment(lib, "d3d8.lib")
#pragma comment(lib, "d3dx8.lib")

ILimage*	MakeD3D8Compliant(IDirect3DDevice8 *Device, D3DFORMAT *DestFormat);
ILenum		GetD3D8Compat(ILenum Format);
D3DFORMAT	GetD3DFormat(ILenum Format);
ILboolean	iD3D8CreateMipmaps(IDirect3DTexture8 *Texture, ILimage *Image);

ILboolean	FormatsDX8Checked = IL_FALSE;
ILboolean	FormatsDX8supported[6] =
	{ IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE };
D3DFORMAT	FormatsDX8[6] =
	{ D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_L8, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5 };


ILboolean ilutD3D8Init()
{

	return IL_TRUE;
}


ILvoid CheckFormatsDX8(IDirect3DDevice8 *Device)
{
	D3DDISPLAYMODE	DispMode;
	HRESULT			hr;
	IDirect3D8		*TestD3D8;
	ILuint			i;

	IDirect3DDevice8_GetDirect3D(Device, (IDirect3D8**)&TestD3D8);
	IDirect3DDevice8_GetDisplayMode(Device, &DispMode);

	for (i = 0; i < 6; i++) {
		hr = IDirect3D8_CheckDeviceFormat(TestD3D8, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, DispMode.Format, 0, D3DRTYPE_TEXTURE, FormatsDX8[i]);
		FormatsDX8supported[i] = SUCCEEDED(hr);
	}

	IDirect3D8_Release(TestD3D8);
	FormatsDX8Checked = IL_TRUE;

	return;
}


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D8TexFromFile(IDirect3DDevice8 *Device, char *FileName, IDirect3DTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadImage(FileName))
		return IL_FALSE;

	*Texture = ilutD3D8Texture(Device);

	return IL_TRUE;
}
#endif//_WIN32_WCE


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D8VolTexFromFile(IDirect3DDevice8 *Device, char *FileName, IDirect3DVolumeTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadImage(FileName))
		return IL_FALSE;

	*Texture = ilutD3D8VolumeTexture(Device);

	return IL_TRUE;
}
#endif//_WIN32_WCE


ILboolean ILAPIENTRY ilutD3D8TexFromFileInMemory(IDirect3DDevice8 *Device, ILvoid *Lump, ILuint Size, IDirect3DTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadL(IL_TYPE_UNKNOWN, Lump, Size))
		return IL_FALSE;

	*Texture = ilutD3D8Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D8VolTexFromFileInMemory(IDirect3DDevice8 *Device, ILvoid *Lump, ILuint Size, IDirect3DVolumeTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadL(IL_TYPE_UNKNOWN, Lump, Size))
		return IL_FALSE;

	*Texture = ilutD3D8VolumeTexture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D8TexFromResource(IDirect3DDevice8 *Device, HMODULE SrcModule, char *SrcResource, IDirect3DTexture8 **Texture)
{
	HRSRC	Resource;
	ILubyte	*Data;

	iBindImageTemp();

	Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
	Data = (ILubyte*)LockResource(Resource);
	if (!ilLoadL(IL_TYPE_UNKNOWN, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP))))
		return IL_FALSE;

	*Texture = ilutD3D8Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D8VolTexFromResource(IDirect3DDevice8 *Device, HMODULE SrcModule, char *SrcResource, IDirect3DVolumeTexture8 **Texture)
{
	HRSRC	Resource;
	ILubyte	*Data;

	iBindImageTemp();

	Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
	Data = (ILubyte*)LockResource(Resource);
	if (!ilLoadL(IL_TYPE_UNKNOWN, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP))))
		return IL_FALSE;

	*Texture = ilutD3D8VolumeTexture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D8TexFromFileHandle(IDirect3DDevice8 *Device, ILHANDLE File, IDirect3DTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadF(IL_TYPE_UNKNOWN, File))
		return IL_FALSE;

	*Texture = ilutD3D8Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D8VolTexFromFileHandle(IDirect3DDevice8 *Device, ILHANDLE File, IDirect3DVolumeTexture8 **Texture)
{
	iBindImageTemp();
	if (!ilLoadF(IL_TYPE_UNKNOWN, File))
		return IL_FALSE;

	*Texture = ilutD3D8VolumeTexture(Device);

	return IL_TRUE;
}


D3DFORMAT D3DGetDXTCNumDX8(ILenum DXTCFormat)
{
	switch (DXTCFormat)
	{
		case IL_DXT1:
			return D3DFMT_DXT1;
		case IL_DXT3:
			return D3DFMT_DXT3;
		case IL_DXT5:
			return D3DFMT_DXT5;
	}

	return 0;
}


IDirect3DTexture8* ILAPIENTRY ilutD3D8Texture(IDirect3DDevice8 *Device)
{
	IDirect3DTexture8 *Texture;
	D3DLOCKED_RECT Rect;
	D3DFORMAT Format;
	ILimage	*Image;
	ILenum	DXTCFormat;
	ILuint	Size;
	ILubyte	*Buffer;

	Image = ilutCurImage = ilGetCurImage();
	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return NULL;
	}

	if (!FormatsDX8Checked)
		CheckFormatsDX8(Device);

	if (ilutGetBoolean(ILUT_D3D_USE_DXTC) && FormatsDX8supported[3] && FormatsDX8supported[4] && FormatsDX8supported[5]) {
		if (ilutCurImage->DxtcData != NULL && ilutCurImage->DxtcSize != 0) {
			Format = D3DGetDXTCNumDX8(ilutCurImage->DxtcFormat);

			if (FAILED(IDirect3DDevice8_CreateTexture(Device, ilutCurImage->Width,
				ilutCurImage->Height, ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format,
				ilutGetInteger(ILUT_D3D_POOL), &Texture)))
					return NULL;
			if (FAILED(IDirect3DTexture8_LockRect(Texture, 0, &Rect, NULL, 0)))
				return NULL;
			memcpy(Rect.pBits, ilutCurImage->DxtcData, ilutCurImage->DxtcSize);
			goto success;
		}

		if (ilutGetBoolean(ILUT_D3D_GEN_DXTC)) {
			DXTCFormat = ilutGetInteger(ILUT_DXTC_FORMAT);

			Size = ilGetDXTCData(NULL, 0, DXTCFormat);
			if (Size != 0) {
				Buffer = (ILubyte*)ialloc(Size);
				if (Buffer == NULL)
					return NULL;
				Size = ilGetDXTCData(Buffer, Size, DXTCFormat);
				if (Size == 0) {
					ifree(Buffer);
					return NULL;
				}

				Format = D3DGetDXTCNumDX8(DXTCFormat);
				if (FAILED(IDirect3DDevice8_CreateTexture(Device, ilutCurImage->Width,
					ilutCurImage->Height, ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format,
					ilutGetInteger(ILUT_D3D_POOL), &Texture))) {
						ifree(Buffer);
						return NULL;
				}
				if (FAILED(IDirect3DTexture8_LockRect(Texture, 0, &Rect, NULL, 0))) {
					ifree(Buffer);
					return NULL;
				}
				memcpy(Rect.pBits, Buffer, Size);
				ifree(Buffer);
				goto success;
			}
		}
	}

	Image = MakeD3D8Compliant(Device, &Format);
	if (Image == NULL) {
		if (Image != ilutCurImage)
			ilCloseImage(Image);
		return NULL;
	}
	if (FAILED(IDirect3DDevice8_CreateTexture(Device, Image->Width, Image->Height,
		ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format, ilutGetInteger(ILUT_D3D_POOL), &Texture))) {
		if (Image != ilutCurImage)
			ilCloseImage(Image);
		return NULL;
	}
	if (FAILED(IDirect3DTexture8_LockRect(Texture, 0, &Rect, NULL, 0)))
		return NULL;
	memcpy(Rect.pBits, Image->Data, Image->SizeOfPlane);

success:
	IDirect3DTexture8_UnlockRect(Texture, 0);
	// Just let D3DX filter for us.
	//D3DXFilterTexture(Texture, NULL, D3DX_DEFAULT, D3DX_FILTER_BOX);
	iD3D8CreateMipmaps(Texture, Image);

	if (Image != ilutCurImage)
		ilCloseImage(Image);

	return Texture;
}


IDirect3DVolumeTexture8* ILAPIENTRY ilutD3D8VolumeTexture(IDirect3DDevice8 *Device)
{
	IDirect3DVolumeTexture8	*Texture;
	D3DLOCKED_BOX	Box;
	D3DFORMAT		Format;
	ILimage			*Image;

	ilutCurImage = ilGetCurImage();
	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return NULL;
	}

	if (!FormatsDX8Checked)
		CheckFormatsDX8(Device);

	Image = MakeD3D8Compliant(Device, &Format);
	if (Image == NULL)
		return NULL;
	if (FAILED(IDirect3DDevice8_CreateVolumeTexture(Device, Image->Width, Image->Height,
		Image->Depth, 1, 0, Format, ilutGetInteger(ILUT_D3D_POOL), &Texture)))
		return NULL;	
	if (FAILED(IDirect3DVolumeTexture8_LockBox(Texture, 0, &Box, NULL, 0)))
		return NULL;

	memcpy(Box.pBits, Image->Data, Image->SizeOfData);
	if (!IDirect3DVolumeTexture8_UnlockBox(Texture, 0))
		return IL_FALSE;

	// We don't want to have mipmaps for such a large image.

	if (Image != ilutCurImage)
		ilCloseImage(Image);

	return Texture;
}


ILimage *MakeD3D8Compliant(IDirect3DDevice8 *Device, D3DFORMAT *DestFormat)
{
	ILimage	*Converted, *Scaled, *CurImage;

	*DestFormat = D3DFMT_A8R8G8B8;

	// Images must be in BGRA format.
	if (ilutCurImage->Format != IL_BGRA) {
		Converted = iConvertImage(ilutCurImage, IL_BGRA, IL_UNSIGNED_BYTE);
		if (Converted == NULL)
			return NULL;
	}
	else {
		Converted = ilutCurImage;
	}

	// Images must have their origin in the upper left.
	if (Converted->Origin != IL_ORIGIN_UPPER_LEFT) {
		CurImage = ilutCurImage;
		ilSetCurImage(Converted);
		iluFlipImage();
		ilSetCurImage(CurImage);
	}

	// Images must have powers-of-2 dimensions.
	if (ilNextPower2(ilutCurImage->Width) != ilutCurImage->Width ||
		ilNextPower2(ilutCurImage->Height) != ilutCurImage->Height ||
		ilNextPower2(ilutCurImage->Depth) != ilutCurImage->Depth) {
			Scaled = iluScale_(Converted, ilNextPower2(ilutCurImage->Width),
						ilNextPower2(ilutCurImage->Height), ilNextPower2(ilutCurImage->Depth));
			if (Converted != ilutCurImage) {
				ilCloseImage(Converted);
			}
			if (Scaled == NULL) {
				return NULL;
			}
			Converted = Scaled;
	}

	return Converted;
}


ILboolean iD3D8CreateMipmaps(IDirect3DTexture8 *Texture, ILimage *Image)
{
	D3DLOCKED_RECT	Rect;
	D3DSURFACE_DESC	Desc;
	ILuint			NumMips, Width, Height, i;
	ILimage			*CurImage, *MipImage, *Temp;

	NumMips = IDirect3DTexture8_GetLevelCount(Texture);
	Width = Image->Width;
	Height = Image->Height;

	CurImage = ilGetCurImage();
	MipImage = ilCopyImage_(CurImage);
	ilSetCurImage(MipImage);
	if (!iluBuildMipmaps()) {
		ilCloseImage(MipImage);
		ilSetCurImage(CurImage);
		return IL_FALSE;
	}
	ilSetCurImage(CurImage);
	Temp = MipImage->Mipmaps;

	// Counts the base texture as 1.
	for (i = 1; i < NumMips && Temp != NULL; i++) {
		if (FAILED(IDirect3DTexture8_LockRect(Texture, i, &Rect, NULL, 0)))
			return IL_FALSE;

		Width = IL_MAX(1, Width / 2);
		Height = IL_MAX(1, Height / 2);

		IDirect3DTexture8_GetLevelDesc(Texture, i, &Desc);
		if (Desc.Width != Width || Desc.Height != Height) {
			IDirect3DTexture8_UnlockRect(Texture, i);
			return IL_FALSE;
		}

		memcpy(Rect.pBits, Temp->Data, Temp->SizeOfData);

		IDirect3DTexture8_UnlockRect(Texture, i);
		Temp = Temp->Next;
	}

	ilCloseImage(MipImage);

	return IL_TRUE;
}


//
// SaveSurfaceToFile.cpp
//
// Copyright (c) 2001 David Galeano
//
// Permission to use, copy, modify and distribute this software
// is hereby granted, provided that both the copyright notice and 
// this permission notice appear in all copies of the software, 
// derivative works or modified versions.
//
// THE AUTHOR ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
// CONDITION AND DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES 
// WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
//

ILAPI ILboolean ILAPIENTRY ilutD3D8LoadSurface(IDirect3DDevice8 *Device, IDirect3DSurface8 *Surface)
{
	HRESULT				hr;
	D3DSURFACE_DESC		d3dsd;
	LPDIRECT3DSURFACE8	SurfaceCopy;
	D3DLOCKED_RECT		d3dLR;
	ILboolean			bHasAlpha;
	ILubyte				*Image, *ImageAux, *Data;
	ILuint				y, x;
	ILushort			dwColor;

	IDirect3DSurface8_GetDesc(Surface, &d3dsd);

	bHasAlpha = (d3dsd.Format == D3DFMT_A8R8G8B8 || d3dsd.Format == D3DFMT_A1R5G5B5);

	if (bHasAlpha) {
		if (!ilTexImage(d3dsd.Width, d3dsd.Height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
	}
	else {
		if (!ilTexImage(d3dsd.Width, d3dsd.Height, 1, 3, IL_BGR, IL_UNSIGNED_BYTE, NULL)) {
			return IL_FALSE;
		}
	}

	hr = IDirect3DDevice8_CreateImageSurface(Device, d3dsd.Width, d3dsd.Height, d3dsd.Format, &SurfaceCopy);
	if (FAILED(hr)) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	hr = IDirect3DDevice8_CopyRects(Device, Surface, NULL, 0, SurfaceCopy, NULL);
	if (FAILED(hr)) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	hr = IDirect3DSurface8_LockRect(SurfaceCopy, &d3dLR, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
	if (FAILED(hr)) {
		IDirect3DSurface8_Release(SurfaceCopy);
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	Image = (ILubyte*)d3dLR.pBits;
	Data = ilutCurImage->Data;

	for (y = 0; y < d3dsd.Height; y++) {
		if (d3dsd.Format == D3DFMT_X8R8G8B8) {
			ImageAux = Image;
			for (x = 0; x < d3dsd.Width; x++) {
				Data[0] = ImageAux[0];
				Data[1] = ImageAux[1];
				Data[2] = ImageAux[2];

				Data += 3;
				ImageAux += 4;
			}
		}
		else if (d3dsd.Format == D3DFMT_A8R8G8B8) {
			memcpy(Data, Image, d3dsd.Width * 4);
		}
		else if (d3dsd.Format == D3DFMT_R5G6B5) {
			ImageAux = Image;
			for (x = 0; x < d3dsd.Width; x++) {
				dwColor = *((ILushort*)ImageAux);

				Data[0] = (ILubyte)((dwColor&0x001f)<<3);
				Data[1] = (ILubyte)(((dwColor&0x7e0)>>5)<<2);
				Data[2] = (ILubyte)(((dwColor&0xf800)>>11)<<3);

				Data += 3;
				ImageAux += 2;
			}
		}
		else if (d3dsd.Format == D3DFMT_X1R5G5B5) {
			ImageAux = Image;
			for (x = 0; x < d3dsd.Width; x++) {
				dwColor = *((ILushort*)ImageAux);

				Data[0] = (ILubyte)((dwColor&0x001f)<<3);
				Data[1] = (ILubyte)(((dwColor&0x3e0)>>5)<<3);
				Data[2] = (ILubyte)(((dwColor&0x7c00)>>10)<<3);

				Data += 3;
				ImageAux += 2;
			}
		}
		else if (d3dsd.Format == D3DFMT_A1R5G5B5) {
			ImageAux = Image;
			for (x = 0; x < d3dsd.Width; x++) {
				dwColor = *((ILushort*)ImageAux);

				Data[0] = (ILubyte)((dwColor&0x001f)<<3);
				Data[1] = (ILubyte)(((dwColor&0x3e0)>>5)<<3);
				Data[2] = (ILubyte)(((dwColor&0x7c00)>>10)<<3);
				Data[3] = (ILubyte)(((dwColor&0x8000)>>15)*255);

				Data += 4;
				ImageAux += 2;
			}
		}

		Image += d3dLR.Pitch;
	}

	IDirect3DSurface8_UnlockRect(SurfaceCopy);
	IDirect3DSurface8_Release(SurfaceCopy);

	return IL_TRUE;
}

#endif//ILUT_USE_DIRECTX8
