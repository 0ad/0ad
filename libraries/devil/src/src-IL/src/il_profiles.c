//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 01/23/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_profiles.c
//
// Description: Colour profile handler
//
//-----------------------------------------------------------------------------

#include "il_internal.h"
#ifndef IL_NO_LCMS

#ifndef _WIN32
	#define NON_WINDOWS 1
	#include <lcms/lcms.h>
#else
//	#ifndef IL_DEBUG
//		#pragma comment(lib, "lcms108.lib")
//	#else
//		#pragma comment(lib, "debug/lcms108.lib")
//	#endif
	#include <lcms.h>
#endif//_WIN32

#endif//IL_NO_LCMS
ILboolean ILAPIENTRY ilApplyProfile(const ILstring InProfile, const ILstring OutProfile)
{
#ifndef IL_NO_LCMS
	cmsHPROFILE		hInProfile, hOutProfile;
	cmsHTRANSFORM	hTransform;
	ILbyte			*Temp;
	ILint			Format=0;

	if (iCurImage == NULL) {
		ilSetError(IL_ILLEGAL_OPERATION);
		return IL_FALSE;
	}

	switch (iCurImage->Type)
	{
		case IL_BYTE:
		case IL_UNSIGNED_BYTE:
			switch (iCurImage->Format)
			{
				case IL_LUMINANCE:
					Format = TYPE_GRAY_8;
					break;
				case IL_RGB:
					Format = TYPE_RGB_8;
					break;
				case IL_BGR:
					Format = TYPE_BGR_8;
					break;
				case IL_RGBA:
					Format = TYPE_RGBA_8;
					break;
				case IL_BGRA:
					Format = TYPE_BGRA_8;
					break;
				default:
					ilSetError(IL_INTERNAL_ERROR);
					return IL_FALSE;
			}
			break;

		case IL_SHORT:
		case IL_UNSIGNED_SHORT:
			switch (iCurImage->Format)
			{
				case IL_LUMINANCE:
					Format = TYPE_GRAY_16;
					break;
				case IL_RGB:
					Format = TYPE_RGB_16;
					break;
				case IL_BGR:
					Format = TYPE_BGR_16;
					break;
				case IL_RGBA:
					Format = TYPE_RGBA_16;
					break;
				case IL_BGRA:
					Format = TYPE_BGRA_16;
					break;
				default:
					ilSetError(IL_INTERNAL_ERROR);
					return IL_FALSE;
			}
			break;

		// These aren't supported right now.
		case IL_INT:
		case IL_UNSIGNED_INT:
		case IL_FLOAT:
		case IL_DOUBLE:
			ilSetError(IL_ILLEGAL_OPERATION);
			return IL_FALSE;
	}


	if (InProfile == NULL) {
		if (!iCurImage->Profile || !iCurImage->ProfileSize) {
			ilSetError(IL_INVALID_PARAM);
			return IL_FALSE;
		}
		hInProfile = iCurImage->Profile;
	}
	else {
		hInProfile = cmsOpenProfileFromFile(InProfile, "r");
	}

	hOutProfile = cmsOpenProfileFromFile(OutProfile, "r");

	hTransform = cmsCreateTransform(hInProfile, Format, hOutProfile, Format, INTENT_PERCEPTUAL, 0);

	Temp = (ILbyte*)ialloc(iCurImage->SizeOfData);
	if (Temp == NULL) {
		return IL_FALSE;
	}

	cmsDoTransform(hTransform, iCurImage->Data, Temp, iCurImage->SizeOfData / 3);

	free(iCurImage->Data);
	iCurImage->Data = Temp;

	cmsDeleteTransform(hTransform);
	if (InProfile != NULL)
		cmsCloseProfile(hInProfile);
	cmsCloseProfile(hOutProfile);

#endif//IL_NO_LCMS

	return IL_TRUE;
}
