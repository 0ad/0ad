//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2001 by Denton Woods
// Last modified: 12/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_stack.c
//
// Description: The main image stack
//
//-----------------------------------------------------------------------------

// Credit goes to John Villar (johnny@reliaschev.com) for making the suggestion
//	of not letting the user use ILimage structs but instead binding images
//	like OpenGL.

#include "il_internal.h"
#include "il_stack.h"


//! Creates Num images and puts their index in Images - similar to glGenTextures().
ILvoid ILAPIENTRY ilGenImages(ILsizei Num, ILuint *Images)
{
	ILsizei	Index = 0;
	iFree	*TempFree = FreeNames;

	if (Num < 1 || Images == NULL) {
		ilSetError(IL_INVALID_VALUE);
		return;
	}

	// No images have been generated yet, so create the image stack.
	if (ImageStack == NULL)
		if (!iEnlargeStack())
			return;

	do {
		if (FreeNames != NULL) {  // If any have been deleted, then reuse their image names.
			while (FreeNames != NULL && ImageStack[FreeNames->Name] != NULL) {
				TempFree = FreeNames->Next;
				ifree(FreeNames);
				FreeNames = TempFree;
			}
			if (FreeNames) {
				TempFree = FreeNames->Next;
				Images[Index] = FreeNames->Name;
				ImageStack[FreeNames->Name] = ilNewImage(1, 1, 1, 1, 1);
				ifree(FreeNames);
				FreeNames = TempFree;
			}
		}

		if (FreeNames == NULL) {  // None have been deleted before.
			if (LastUsed >= StackSize)
				if (!iEnlargeStack())
					return;
			Images[Index] = LastUsed;
			// Must be all 1's instead of 0's, because some functions would divide by 0.
			ImageStack[LastUsed] = ilNewImage(1, 1, 1, 1, 1);
			LastUsed++;
		}
	} while (++Index < Num);

	return;
}


//! Makes Image the current active image - similar to glBindTexture().
ILvoid ILAPIENTRY ilBindImage(ILuint Image)
{
	if (ImageStack == NULL || StackSize == 0) {
		if (!iEnlargeStack()) {
			return;
		}
	}

	// If the user requests a high image name.
	while (Image >= StackSize) {
		if (!iEnlargeStack()) {
			return;
		}
	}

	if (ImageStack[Image] == NULL) {
		ImageStack[Image] = ilNewImage(1, 1, 1, 1, 1);
		if (Image >= LastUsed) // >= ?
			LastUsed = Image + 1;
	}

	iCurImage = ImageStack[Image];
	CurName = Image;

	ParentImage = IL_TRUE;

	return;
}


//! Deletes Num images from the image stack - similar to glDeleteTextures().
ILvoid ILAPIENTRY ilDeleteImages(ILsizei Num, const ILuint *Images)
{
	iFree	*Temp = FreeNames;
	ILuint	Index = 0;

	if (Num < 1) {
		//ilSetError(IL_INVALID_VALUE);
		return;
	}
	if (StackSize == 0)
		return;

	do {
		if (Images[Index] > 0 && Images[Index] < LastUsed) {  // <= ?
			/*if (FreeNames != NULL) {  // Terribly inefficient
				Temp = FreeNames;
				do {
					if (Temp->Name == Images[Index]) {
						continue;  // Sufficient?
					}
				} while ((Temp = Temp->Next));
			}*/

			// Already has been deleted or was never used.
			if (ImageStack[Images[Index]] == NULL)
				continue;

			// Find out if current image - if so, set to default image zero.
			if (Images[Index] == CurName || Images[Index] == 0) {
				iCurImage = ImageStack[0];
				CurName = 0;
			}
			
			// Should *NOT* be NULL here!
			ilCloseImage(ImageStack[Images[Index]]);
			ImageStack[Images[Index]] = NULL;

			// Add to head of list - works for empty and non-empty lists
			Temp = (iFree*)ialloc(sizeof(iFree));
			if (!Temp) {
				return;
			}
			Temp->Name = Images[Index];
			Temp->Next = FreeNames;
			FreeNames = Temp;
		}
		/*else {  // Shouldn't set an error...just continue onward.
			ilSetError(IL_ILLEGAL_OPERATION);
		}*/
	} while (++Index < (ILuint)Num);

	return;
}


//! Checks if Image is a valid ilGenImages-generated image (like glIsTexture()).
ILboolean ILAPIENTRY ilIsImage(ILuint Image)
{
	//iFree *Temp = FreeNames;

	if (ImageStack == NULL)
		return IL_FALSE;
	if (Image >= LastUsed || Image == 0)
		return IL_FALSE;

	/*do {
		if (Temp->Name == Image)
			return IL_FALSE;
	} while ((Temp = Temp->Next));*/

	if (ImageStack[Image] == NULL)  // Easier check.
		return IL_FALSE;

	return IL_TRUE;
}


//! Closes Image and frees all memory associated with it.
ILAPI ILvoid ILAPIENTRY ilCloseImage(ILimage *Image)
{
	if (Image == NULL)
		return;

	if (Image->Data != NULL) {
		ifree(Image->Data);
		Image->Data = NULL;
	}

	if (Image->Pal.Palette != NULL && Image->Pal.PalSize > 0 && Image->Pal.PalType != IL_PAL_NONE) {
		ifree(Image->Pal.Palette);
		Image->Pal.Palette = NULL;
	}

	if (Image->Next != NULL) {
		ilCloseImage(Image->Next);
		Image->Next = NULL;
	}

	if (Image->Mipmaps != NULL) {
		ilCloseImage(Image->Mipmaps);
		Image->Mipmaps = NULL;
	}

	if (Image->Layers != NULL) {
		ilCloseImage(Image->Layers);
		Image->Layers = NULL;
	}

	if (Image->AnimList != NULL && Image->AnimSize != 0) {
		ifree(Image->AnimList);
		Image->AnimList = NULL;
	}

	if (Image->Profile != NULL && Image->ProfileSize != 0) {
		ifree(Image->Profile);
		Image->Profile = NULL;
		Image->ProfileSize = 0;
	}

	if (Image->DxtcData != NULL && Image->DxtcFormat != IL_DXT_NO_COMP) {
		ifree(Image->DxtcData);
		Image->DxtcData = NULL;
		Image->DxtcFormat = IL_DXT_NO_COMP;
		Image->DxtcSize = 0;
	}

	ifree(Image);
	Image = NULL;

	return;
}


ILAPI ILboolean ILAPIENTRY ilIsValidPal(ILpal *Palette)
{
	if (Palette == NULL)
		return IL_FALSE;
	if (Palette->PalSize == 0 || Palette->Palette == NULL)
		return IL_FALSE;
	switch (Palette->PalType)
	{
		case IL_PAL_RGB24:
		case IL_PAL_RGB32:
		case IL_PAL_RGBA32:
		case IL_PAL_BGR24:
		case IL_PAL_BGR32:
		case IL_PAL_BGRA32:
			return IL_TRUE;
	}
	return IL_FALSE;
}


//! Closes Palette and frees all memory associated with it.
ILAPI ILvoid ILAPIENTRY ilClosePal(ILpal *Palette)
{
	if (Palette == NULL)
		return;
	if (!ilIsValidPal(Palette))
		return;
	ifree(Palette->Palette);
	ifree(Palette);
	return;
}


ILimage *iGetBaseImage()
{
	return ImageStack[ilGetCurName()];
}


//! Sets the current mipmap level
ILboolean ILAPIENTRY ilActiveMipmap(ILuint Number)
{
	ILuint Current;

	//ParentImage = IL_TRUE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (Number == 0) {
		iCurImage = ImageStack[ilGetCurName()];
		return IL_TRUE;
	}

	if (Number > iCurImage->NumMips) {
		ilSetError(IL_ILLEGAL_OPERATION);
		//iCurImage = ImageStack[ilGetCurName()];
		return IL_FALSE;
	}

	iCurImage = iCurImage->Mipmaps;
	Number--;  // Skip 0 (parent image)
	for (Current = 0; Current < Number; Current++) {
		iCurImage = iCurImage->Next;
		if (iCurImage == NULL) {
			ilSetError(IL_INTERNAL_ERROR);
			iCurImage = ImageStack[ilGetCurName()];
			return IL_FALSE;
		}
	}

	ParentImage = IL_FALSE;

	return IL_TRUE;
}


//! Used for setting the current image if it is an animation.
ILboolean ILAPIENTRY ilActiveImage(ILuint Number)
{
	ILuint Current;

	//ParentImage = IL_TRUE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (Number == 0) {
		iCurImage = ImageStack[ilGetCurName()];
		return IL_TRUE;
	}

	if (Number > iCurImage->NumNext) {
		ilSetError(IL_ILLEGAL_OPERATION);
		//iCurImage = ImageStack[ilGetCurName()];
		return IL_FALSE;
	}

	iCurImage = iCurImage->Next;
	Number--;  // Skip 0 (parent image)
	for (Current = 0; Current < Number; Current++) {
		iCurImage = iCurImage->Next;
		if (iCurImage == NULL) {
			ilSetError(IL_INTERNAL_ERROR);
			iCurImage = ImageStack[ilGetCurName()];
			return IL_FALSE;
		}
	}

	ParentImage = IL_FALSE;

	return IL_TRUE;
}


//! Used for setting the current layer if layers exist.
ILboolean ILAPIENTRY ilActiveLayer(ILuint Number)
{
	ILuint Current;

	//ParentImage = IL_TRUE;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}
	if (Number == 0) {
		iCurImage = ImageStack[ilGetCurName()];
		return IL_TRUE;
	}

	if (Number > iCurImage->NumLayers) {
		ilSetError(IL_ILLEGAL_OPERATION);
		//iCurImage = ImageStack[ilGetCurName()];
		return IL_FALSE;
	}

	iCurImage = iCurImage->Layers;
	Number--;  // Skip 0 (parent image)
	for (Current = 0; Current < Number; Current++) {
		iCurImage = iCurImage->Layers;
		if (iCurImage == NULL) {
			ilSetError(IL_INTERNAL_ERROR);
			iCurImage = ImageStack[ilGetCurName()];
			return IL_FALSE;
		}
	}

	ParentImage = IL_FALSE;

	return IL_TRUE;
}


ILuint ILAPIENTRY ilCreateSubImage(ILenum Type, ILuint Num)
{
	ILimage	*SubImage;
	ILuint	Count = 1;  // Create one before we go in the loop.

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return 0;
	}
	if (Num == 0) {
		//ilSetError(IL_INVALID_PARAM);
		return 0;
	}

	switch (Type)
	{
		case IL_SUB_NEXT:
			if (iCurImage->Next)
				ilCloseImage(iCurImage->Next);
			iCurImage->Next = ilNewImage(1, 1, 1, 1, 1);
			iCurImage->NumNext = Num;
			SubImage = iCurImage->Next;
			break;

		case IL_SUB_MIPMAP:
			if (iCurImage->Mipmaps)
				ilCloseImage(iCurImage->Mipmaps);
			iCurImage->Mipmaps = ilNewImage(1, 1, 1, 1, 1);
			iCurImage->NumMips = Num;
			SubImage = iCurImage->Mipmaps;
			break;

		case IL_SUB_LAYER:
			if (iCurImage->Layers)
				ilCloseImage(iCurImage->Layers);
			iCurImage->Layers = ilNewImage(1, 1, 1, 1, 1);
			iCurImage->NumLayers = Num;
			SubImage = iCurImage->Layers;
			break;

		default:
			ilSetError(IL_INVALID_ENUM);
			return IL_FALSE;
	}

	if (SubImage == NULL) {
		return 0;
	}

	for (; Count < Num; Count++) {
		SubImage->Next = ilNewImage(1, 1, 1, 1, 1);
		SubImage = SubImage->Next;
		if (SubImage == NULL)
			return Count;
	}

	return Count;
}


// Returns the current index.
ILAPI ILuint ILAPIENTRY ilGetCurName()
{
	if (iCurImage == NULL || ImageStack == NULL || StackSize == 0)
		return 0;
	return CurName;
}


// Returns the current image.
ILAPI ILimage* ILAPIENTRY ilGetCurImage()
{
	return iCurImage;
}


// To be only used when the original image is going to be set back almost immediately.
ILAPI ILvoid ILAPIENTRY ilSetCurImage(ILimage *Image)
{
	iCurImage = Image;
	return;
}


// Completely replaces the current image and the version in the image stack.
ILAPI ILvoid ILAPIENTRY ilReplaceCurImage(ILimage *Image)
{
	if (iCurImage) {
		ilActiveImage(0);
		ilCloseImage(iCurImage);
	}
	ImageStack[ilGetCurName()] = Image;
	iCurImage = Image;
	ParentImage = IL_TRUE;
	return;
}


// Like realloc but sets new memory to 0.
ILvoid* ILAPIENTRY ilRecalloc(ILvoid *Ptr, ILuint OldSize, ILuint NewSize)
{
	ILvoid *Temp = ialloc(NewSize);
	ILuint CopySize = (OldSize < NewSize) ? OldSize : NewSize;

	if (Temp != NULL) {
		if (Ptr != NULL) {
			memcpy(Temp, Ptr, CopySize);
			ifree(Ptr);
		}

		Ptr = Temp;

		if (OldSize < NewSize)
			imemclear((ILubyte*)Temp + OldSize, NewSize - OldSize);
	}

	return Temp;
}


// Internal function to enlarge the image stack by I_STACK_INCREMENT members.
ILboolean iEnlargeStack()
{
	// 02-05-2001:  Moved from ilGenImages().
	// Puts the cleanup function on the exit handler once.
	if (!OnExit) {
		#ifdef _MEM_DEBUG
			AddToAtexit();  // So iFreeMem doesn't get called after unfreed information.
		#endif//_MEM_DEBUG
		atexit((void*)ilShutDown);
		OnExit = IL_TRUE;
	}

	if (!(ImageStack = ilRecalloc(ImageStack, StackSize * sizeof(ILimage*), (StackSize + I_STACK_INCREMENT) * sizeof(ILimage*)))) {
		return IL_FALSE;
	}
	StackSize += I_STACK_INCREMENT;
	return IL_TRUE;
}


ILboolean IsInit = IL_FALSE;

// ONLY call at startup.
ILvoid ILAPIENTRY ilInit()
{
	ilSetMemory(NULL, NULL);  // Do first, since it handles allocations.
	ilSetError(IL_NO_ERROR);
	ilDefaultStates();  // Set states to their defaults.
	// Sets default file-reading callbacks.
	ilResetRead();
	ilResetWrite();
#ifndef _WIN32_WCE
	atexit((void*)ilRemoveRegistered);
#endif//_WIN32_WCE
	//ilShutDown();
	iSetImage0();  // Beware!  Clears all existing textures!
	iBindImageTemp();  // Go ahead and create the temporary image.
	IsInit = IL_TRUE;
	return;
}


// Frees any extra memory in the stack.
//	- Called on exit
ILvoid ILAPIENTRY ilShutDown()
{
	static ILboolean BeenCalled = IL_FALSE;
	iFree *TempFree = FreeNames;
	ILuint i;

	if (!IsInit) {  // Prevent from being called when not initialized.
		ilSetError(IL_ILLEGAL_OPERATION);
		return;
	}

	while (TempFree != NULL) {
		FreeNames = TempFree->Next;
		ifree(TempFree);
		TempFree = FreeNames;
	}

	//for (i = 0; i < LastUsed; i++) {
	for (i = 0; i < StackSize; i++) {
		if (ImageStack[i] != NULL)
			ilCloseImage(ImageStack[i]);
	}

	if (ImageStack)
		ifree(ImageStack);
	ImageStack = NULL;
	LastUsed = 0;
	StackSize = 0;
	BeenCalled = IL_TRUE;

	return;
}


// Initializes the image stack's first entry (default image) -- ONLY CALL ONCE!
ILvoid iSetImage0()
{
	if (ImageStack == NULL)
		if (!iEnlargeStack())
			return;

	LastUsed = 1;
	CurName = 0;
	ParentImage = IL_TRUE;
	if (!ImageStack[0])
		ImageStack[0] = ilNewImage(1, 1, 1, 1, 1);
	iCurImage = ImageStack[0];
	ilDefaultImage();

	return;
}


ILAPI ILvoid ILAPIENTRY iBindImageTemp()
{
	if (ImageStack == NULL || StackSize <= 1)
		if (!iEnlargeStack())
			return;

	if (LastUsed <2 )
		LastUsed = 2;
	CurName = 1;
	ParentImage = IL_TRUE;
	if (!ImageStack[1])
		ImageStack[1] = ilNewImage(1, 1, 1, 1, 1);
	iCurImage = ImageStack[1];

	return;
}
