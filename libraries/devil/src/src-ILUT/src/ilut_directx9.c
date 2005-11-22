//-----------------------------------------------------------------------------
//
// ImageLib Utility Toolkit Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 12/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILUT/src/ilut_directx9.c
//
// Description: DirectX 9 functions for textures
//
//-----------------------------------------------------------------------------


#include "ilut_internal.h"
#ifdef ILUT_USE_DIRECTX9

#include <d3d9.h>
//#include <d3dx9tex.h>
#pragma comment(lib, "d3d9.lib")
//#pragma comment(lib, "d3dx9.lib")

ILimage*	MakeD3D9Compliant(IDirect3DDevice9 *Device, D3DFORMAT *DestFormat);
ILenum		GetD3D9Compat(ILenum Format);
//D3DFORMAT	GetD3DFormat(ILenum Format);
D3DFORMAT	D3DGetDXTCNumDX9(ILenum DXTCFormat);
ILenum		D3DGetDXTCFormat(D3DFORMAT DXTCNum);
ILboolean	iD3D9CreateMipmaps(IDirect3DTexture9 *Texture, ILimage *Image);
IDirect3DTexture9* iD3DMakeTexture( IDirect3DDevice9 *Device, void *Data, ILuint DLen, ILuint Width, ILuint Height, D3DFORMAT Format, D3DPOOL Pool, ILuint Levels );

ILboolean	FormatsDX9Checked = IL_FALSE;
ILboolean	FormatsDX9supported[6] =
	{ IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE, IL_FALSE };
D3DFORMAT	FormatsDX9[6] =
	{ D3DFMT_R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_L8, D3DFMT_DXT1, D3DFMT_DXT3, D3DFMT_DXT5 };


ILboolean ilutD3D9Init()
{

	return IL_TRUE;
}


ILvoid CheckFormatsDX9(IDirect3DDevice9 *Device)
{
	D3DDISPLAYMODE	DispMode;
	HRESULT			hr;
	IDirect3D9		*TestD3D9;
	ILuint			i;

	IDirect3DDevice9_GetDirect3D(Device, (IDirect3D9**)&TestD3D9);
	IDirect3DDevice9_GetDisplayMode(Device, 0, &DispMode);

	for (i = 0; i < 6; i++) {
		hr = IDirect3D9_CheckDeviceFormat(TestD3D9, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, DispMode.Format, 0, D3DRTYPE_TEXTURE, FormatsDX9[i]);
		FormatsDX9supported[i] = SUCCEEDED(hr);
	}

	IDirect3D9_Release(TestD3D9);
	FormatsDX9Checked = IL_TRUE;

	return;
}


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D9TexFromFile(IDirect3DDevice9 *Device, char *FileName, IDirect3DTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadImage(FileName))
		return IL_FALSE;

	*Texture = ilutD3D9Texture(Device);

	return IL_TRUE;
}
#endif//_WIN32_WCE


#ifndef _WIN32_WCE
ILboolean ILAPIENTRY ilutD3D9VolTexFromFile(IDirect3DDevice9 *Device, char *FileName, IDirect3DVolumeTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadImage(FileName))
		return IL_FALSE;

	*Texture = ilutD3D9VolumeTexture(Device);

	return IL_TRUE;
}
#endif//_WIN32_WCE


ILboolean ILAPIENTRY ilutD3D9TexFromFileInMemory(IDirect3DDevice9 *Device, ILvoid *Lump, ILuint Size, IDirect3DTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadL(IL_TYPE_UNKNOWN, Lump, Size))
		return IL_FALSE;

	*Texture = ilutD3D9Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9VolTexFromFileInMemory(IDirect3DDevice9 *Device, ILvoid *Lump, ILuint Size, IDirect3DVolumeTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadL(IL_TYPE_UNKNOWN, Lump, Size))
		return IL_FALSE;

	*Texture = ilutD3D9VolumeTexture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9TexFromResource(IDirect3DDevice9 *Device, HMODULE SrcModule, char *SrcResource, IDirect3DTexture9 **Texture)
{
	HRSRC	Resource;
	ILubyte	*Data;

	iBindImageTemp();

	Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
	Data = (ILubyte*)LockResource(Resource);
	if (!ilLoadL(IL_TYPE_UNKNOWN, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP))))
		return IL_FALSE;

	*Texture = ilutD3D9Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9VolTexFromResource(IDirect3DDevice9 *Device, HMODULE SrcModule, char *SrcResource, IDirect3DVolumeTexture9 **Texture)
{
	HRSRC	Resource;
	ILubyte	*Data;

	iBindImageTemp();

	Resource = (HRSRC)LoadResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP));
	Data = (ILubyte*)LockResource(Resource);
	if (!ilLoadL(IL_TYPE_UNKNOWN, Data, SizeofResource(SrcModule, FindResource(SrcModule, SrcResource, RT_BITMAP))))
		return IL_FALSE;

	*Texture = ilutD3D9VolumeTexture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9TexFromFileHandle(IDirect3DDevice9 *Device, ILHANDLE File, IDirect3DTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadF(IL_TYPE_UNKNOWN, File))
		return IL_FALSE;

	*Texture = ilutD3D9Texture(Device);

	return IL_TRUE;
}


ILboolean ILAPIENTRY ilutD3D9VolTexFromFileHandle(IDirect3DDevice9 *Device, ILHANDLE File, IDirect3DVolumeTexture9 **Texture)
{
	iBindImageTemp();
	if (!ilLoadF(IL_TYPE_UNKNOWN, File))
		return IL_FALSE;

	*Texture = ilutD3D9VolumeTexture(Device);

	return IL_TRUE;
}


D3DFORMAT D3DGetDXTCNumDX9(ILenum DXTCFormat)
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


ILenum D3DGetDXTCFormat(D3DFORMAT DXTCNum)
{
	switch (DXTCNum)
	{
		case D3DFMT_DXT1:
			return IL_DXT1;
		case D3DFMT_DXT3:
			return IL_DXT3;
		case D3DFMT_DXT5:
			return IL_DXT5;
	}

	return 0;
}


IDirect3DTexture9* iD3DMakeTexture( IDirect3DDevice9 *Device, void *Data, ILuint DLen, ILuint Width, ILuint Height, D3DFORMAT Format, D3DPOOL Pool, ILuint Levels )
{
	IDirect3DTexture9 *Texture;
	D3DLOCKED_RECT Rect;
	
	if (FAILED(IDirect3DDevice9_CreateTexture(Device, Width, Height, Levels,
			0, Format, Pool, &Texture, NULL)))
		return NULL;
	if (FAILED(IDirect3DTexture9_LockRect(Texture, 0, &Rect, NULL, 0)))
		return NULL;
	memcpy(Rect.pBits, Data, DLen);
	IDirect3DTexture9_UnlockRect(Texture, 0);
	
	return Texture;
}

IDirect3DTexture9* ILAPIENTRY ilutD3D9Texture(IDirect3DDevice9 *Device)
{
	IDirect3DTexture9 *Texture;
//	D3DLOCKED_RECT Rect;
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

	if (!FormatsDX9Checked)
		CheckFormatsDX9(Device);

	if (ilutGetBoolean(ILUT_D3D_USE_DXTC) && FormatsDX9supported[3] && FormatsDX9supported[4] && FormatsDX9supported[5]) {
		if (ilutCurImage->DxtcData != NULL && ilutCurImage->DxtcSize != 0) {
			ILuint	dxtcFormat = ilutGetInteger(ILUT_DXTC_FORMAT);
			Format = D3DGetDXTCNumDX9(ilutCurImage->DxtcFormat);
			ilutSetInteger(ILUT_DXTC_FORMAT, ilutCurImage->DxtcFormat);

			Texture = iD3DMakeTexture(Device, ilutCurImage->DxtcData, ilutCurImage->DxtcSize,
				ilutCurImage->Width, ilutCurImage->Height, Format,
				ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT ? D3DPOOL_SYSTEMMEM : ilutGetInteger(ILUT_D3D_POOL), ilutGetInteger(ILUT_D3D_MIPLEVELS));
			if (!Texture)
				return NULL;
			iD3D9CreateMipmaps(Texture, Image);
			if (ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT) {
				IDirect3DTexture9 *SysTex = Texture;
				// copy texture to device memory
				if (FAILED(IDirect3DDevice9_CreateTexture(Device, ilutCurImage->Width,
						ilutCurImage->Height, ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format,
						D3DPOOL_DEFAULT, &Texture, NULL))) {
					IDirect3DTexture9_Release(SysTex);
					return NULL;
				}
				if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
					IDirect3DTexture9_Release(SysTex);
					return NULL;
				}
				IDirect3DTexture9_Release(SysTex);
			}
			ilutSetInteger(ILUT_DXTC_FORMAT, dxtcFormat);

			goto success;
		}

		if (ilutGetBoolean(ILUT_D3D_GEN_DXTC)) {
			DXTCFormat = ilutGetInteger(ILUT_DXTC_FORMAT);

/*
Image = MakeD3D9Compliant(Device, &Format);
			if (Image == NULL) {
				if (Image != ilutCurImage)
					ilCloseImage(Image);
				return NULL;
			}
*/

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

				Format = D3DGetDXTCNumDX9(DXTCFormat);
				Texture = iD3DMakeTexture(Device, Buffer, Size,
					ilutCurImage->Width, ilutCurImage->Height, Format,
					ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT ? D3DPOOL_SYSTEMMEM : ilutGetInteger(ILUT_D3D_POOL), ilutGetInteger(ILUT_D3D_MIPLEVELS));
				if (!Texture)
					return NULL;
				iD3D9CreateMipmaps(Texture, Image);
				if (ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT) {
					IDirect3DTexture9 *SysTex = Texture;
					
					if (FAILED(IDirect3DDevice9_CreateTexture(Device, ilutCurImage->Width,
							ilutCurImage->Height, ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format,
							D3DPOOL_DEFAULT, &Texture, NULL))) {
						IDirect3DTexture9_Release(SysTex);
						return NULL;
					}
					if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
						IDirect3DTexture9_Release(SysTex);
						return NULL;
					}
					IDirect3DTexture9_Release(SysTex);
				}
				
				goto success;
			}
		}
	}

	Image = MakeD3D9Compliant(Device, &Format);
	if (Image == NULL) {
		if (Image != ilutCurImage)
			ilCloseImage(Image);
		return NULL;
	}

	Texture = iD3DMakeTexture(Device, Image->Data, Image->SizeOfPlane,
		Image->Width, Image->Height, Format,
		ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT ? D3DPOOL_SYSTEMMEM : ilutGetInteger(ILUT_D3D_POOL), ilutGetInteger(ILUT_D3D_MIPLEVELS));
	if (!Texture)
		return NULL;
	iD3D9CreateMipmaps(Texture, Image);
	if (ilutGetInteger(ILUT_D3D_POOL) == D3DPOOL_DEFAULT) {
		IDirect3DTexture9 *SysTex = Texture;
		// create texture in system memory
		if (FAILED(IDirect3DDevice9_CreateTexture(Device, Image->Width,
				Image->Height, ilutGetInteger(ILUT_D3D_MIPLEVELS), 0, Format,
				ilutGetInteger(ILUT_D3D_POOL), &Texture, NULL))) {
			IDirect3DTexture9_Release(SysTex);
			return NULL;
		}
		if (FAILED(IDirect3DDevice9_UpdateTexture(Device, (LPDIRECT3DBASETEXTURE9)SysTex, (LPDIRECT3DBASETEXTURE9)Texture))) {
			IDirect3DTexture9_Release(SysTex);
			return NULL;
		}
		IDirect3DTexture9_Release(SysTex);
	}
//	if (Image != ilutCurImage)
//		ilCloseImage(Image);

success:

	if (Image != ilutCurImage)
		ilCloseImage(Image);

	return Texture;
}


IDirect3DVolumeTexture9* ILAPIENTRY ilutD3D9VolumeTexture(IDirect3DDevice9 *Device)
{
	IDirect3DVolumeTexture9	*Texture;
	D3DLOCKED_BOX	Box;
	D3DFORMAT		Format;
	ILimage			*Image;

	ilutCurImage = ilGetCurImage();
	if (ilutCurImage == NULL) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return NULL;
	}

	if (!FormatsDX9Checked)
		CheckFormatsDX9(Device);

	Image = MakeD3D9Compliant(Device, &Format);
	if (Image == NULL)
		return NULL;
	if (FAILED(IDirect3DDevice9_CreateVolumeTexture(Device, Image->Width, Image->Height,
		Image->Depth, 1, 0, Format, ilutGetInteger(ILUT_D3D_POOL), &Texture, NULL)))
		return NULL;	
	if (FAILED(IDirect3DVolumeTexture9_LockBox(Texture, 0, &Box, NULL, 0)))
		return NULL;

	memcpy(Box.pBits, Image->Data, Image->SizeOfData);
	if (!IDirect3DVolumeTexture9_UnlockBox(Texture, 0))
		return IL_FALSE;

	// We don't want to have mipmaps for such a large image.

	if (Image != ilutCurImage)
		ilCloseImage(Image);

	return Texture;
}


ILimage *MakeD3D9Compliant(IDirect3DDevice9 *Device, D3DFORMAT *DestFormat)
{
	ILuint	color;
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

	// perform alpha key on images if requested
	color=ilutGetInteger(ILUT_D3D_ALPHA_KEY_COLOR);
	if(color>=0)
	{
	ILubyte *data;
	ILubyte *maxdata;
	ILuint t;

		data=(Converted->Data);
		maxdata=(Converted->Data+Converted->SizeOfData);
		while(data<maxdata)
		{
			t= (data[2]<<16) + (data[1]<<8) + (data[0]) ;

			if((t&0x00ffffff)==(color&0x00ffffff))
			{
				data[0]=0;
				data[1]=0;
				data[2]=0;
				data[3]=0;
			}
			data+=4;
		}
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


ILboolean iD3D9CreateMipmaps(IDirect3DTexture9 *Texture, ILimage *Image)
{
	D3DLOCKED_RECT	Rect;
	D3DSURFACE_DESC	Desc;
	ILuint			NumMips, Width, Height, i;
	ILimage			*CurImage, *MipImage, *Temp;
	ILenum			DXTCFormat;
	ILuint			Size;
	ILubyte			*Buffer;
	ILboolean		useDXTC = IL_FALSE;

	NumMips = IDirect3DTexture9_GetLevelCount(Texture);
	Width = Image->Width;
	Height = Image->Height;

	if (NumMips == 1)
		return IL_TRUE;
		
	CurImage = ilGetCurImage();
	MipImage = Image;
	if (MipImage->NumMips != NumMips-1) {
		MipImage = ilCopyImage_(Image);
		ilSetCurImage(MipImage);
		if (!iluBuildMipmaps()) {
			ilCloseImage(MipImage);
			ilSetCurImage(CurImage);
			return IL_FALSE;
		}
	}
//	ilSetCurImage(CurImage);
	Temp = MipImage->Mipmaps;

	if (ilutGetBoolean(ILUT_D3D_USE_DXTC) && FormatsDX9supported[3] && FormatsDX9supported[4] && FormatsDX9supported[5])
		useDXTC = IL_TRUE;

	// Counts the base texture as 1.
	for (i = 1; i < NumMips && Temp != NULL; i++) {
		ilSetCurImage(Temp);
		if (FAILED(IDirect3DTexture9_LockRect(Texture, i, &Rect, NULL, 0)))
			return IL_FALSE;

		Width = IL_MAX(1, Width / 2);
		Height = IL_MAX(1, Height / 2);

		IDirect3DTexture9_GetLevelDesc(Texture, i, &Desc);
		if (Desc.Width != Width || Desc.Height != Height) {
			IDirect3DTexture9_UnlockRect(Texture, i);
			return IL_FALSE;
		}

		if (useDXTC) {
			if (Temp->DxtcData != NULL && Temp->DxtcSize != 0) {
				memcpy(Rect.pBits, Temp->DxtcData, Temp->DxtcSize);
			} else if (ilutGetBoolean(ILUT_D3D_GEN_DXTC)) {
				DXTCFormat = ilutGetInteger(ILUT_DXTC_FORMAT);

				Size = ilGetDXTCData(NULL, 0, DXTCFormat);
				if (Size != 0) {
					Buffer = (ILubyte*)ialloc(Size);
					if (Buffer == NULL) {
						IDirect3DTexture9_UnlockRect(Texture, i);
						return IL_FALSE;
					}
					Size = ilGetDXTCData(Buffer, Size, DXTCFormat);
					if (Size == 0) {
						ifree(Buffer);
						IDirect3DTexture9_UnlockRect(Texture, i);
						return IL_FALSE;
					}
					memcpy(Rect.pBits, Buffer, Size);
				} else {
					IDirect3DTexture9_UnlockRect(Texture, i);
					return IL_FALSE;
				}
			} else {
				IDirect3DTexture9_UnlockRect(Texture, i);
				return IL_FALSE;
			}
		} else
			memcpy(Rect.pBits, Temp->Data, Temp->SizeOfData);

		IDirect3DTexture9_UnlockRect(Texture, i);
		Temp = Temp->Next;
	}

	if (MipImage != Image)
		ilCloseImage(MipImage);
	ilSetCurImage(CurImage);

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

ILAPI ILboolean ILAPIENTRY ilutD3D9LoadSurface(IDirect3DDevice9 *Device, IDirect3DSurface9 *Surface)
{
	HRESULT				hr;
	D3DSURFACE_DESC		d3dsd;
	LPDIRECT3DSURFACE9	SurfaceCopy;
	D3DLOCKED_RECT		d3dLR;
	ILboolean			bHasAlpha;
	ILubyte				*Image, *ImageAux, *Data;
	ILuint				y, x;
	ILushort			dwColor;

	IDirect3DSurface9_GetDesc(Surface, &d3dsd);

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

	hr = IDirect3DDevice9_CreateOffscreenPlainSurface(Device, d3dsd.Width, d3dsd.Height, d3dsd.Format, D3DPOOL_SCRATCH, &SurfaceCopy, NULL);
	if (FAILED(hr)) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	hr = IDirect3DDevice9_StretchRect(Device, Surface, NULL, SurfaceCopy, NULL, D3DTEXF_NONE);
	if (FAILED(hr)) {
		ilSetError(ILUT_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	hr = IDirect3DSurface9_LockRect(SurfaceCopy, &d3dLR, NULL, D3DLOCK_NO_DIRTY_UPDATE | D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY);
	if (FAILED(hr)) {
		IDirect3DSurface9_Release(SurfaceCopy);
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

	IDirect3DSurface9_UnlockRect(SurfaceCopy);
	IDirect3DSurface9_Release(SurfaceCopy);

	return IL_TRUE;
}

#endif//ILUT_USE_DIRECTX9
