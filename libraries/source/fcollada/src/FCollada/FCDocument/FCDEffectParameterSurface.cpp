/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDEffectPass.h"
#include "FCDocument/FCDEffectProfile.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterSurface.h"
#include "FCDocument/FCDImage.h"

//
// FCDEffectParameterSurface
//

ImplementObjectType(FCDEffectParameterSurface);
ImplementParameterObjectNoCtr(FCDEffectParameterSurface, FCDImage, images);

// surface type parameter
FCDEffectParameterSurface::FCDEffectParameterSurface(FCDocument* document)
:	FCDEffectParameter(document)
,	InitializeParameterNoArg(images)
,	initMethod(NULL)
,	format("A8R8G8B8")
,	formatHint(NULL)
,	size(FMVector3::Zero)
,	viewportRatio(1.0f)
,	mipLevelCount(0)
,	generateMipmaps(false)
,	type("2D")
{
}

FCDEffectParameterSurface::~FCDEffectParameterSurface()
{
	SAFE_DELETE(initMethod);
	SAFE_DELETE(formatHint);
	names.clear();
}

void FCDEffectParameterSurface::SetInitMethod(FCDEffectParameterSurfaceInit* method)
{
	SAFE_DELETE(initMethod);
	initMethod = method;
	SetNewChildFlag();
}

// Retrieves the index that matches the given image.
size_t FCDEffectParameterSurface::FindImage(const FCDImage* image) const
{
	const FCDImage** it = images.find(image);
	if (it != images.end()) return it - images.begin();
	else return (size_t) ~0;
}

// Adds an image to the list.
size_t FCDEffectParameterSurface::AddImage(FCDImage* image, size_t index)
{
	size_t exists = FindImage(image);
	if (exists == (size_t) ~0)
	{
		if (index == (size_t) ~0)
		{
			index = images.size();
			images.push_back(image);
		}
		else
		{
			FUAssert(index > images.size(), return (size_t) ~0);
			images.insert(index, image);
		}
		SetNewChildFlag();
	}
	return index;
}

// Removes an image from the list.
void FCDEffectParameterSurface::RemoveImage(FCDImage* image)
{
	size_t index = FindImage(image);
	if (index != (size_t) ~0)
	{
		images.erase(index);

		if (initMethod != NULL && initMethod->GetInitType() == FCDEffectParameterSurfaceInitFactory::CUBE)
		{
			// Shift down all the indexes found within the cube map initialization.
			FCDEffectParameterSurfaceInitCube* cube = (FCDEffectParameterSurfaceInitCube*) initMethod;
			for (UInt16List::iterator itI = cube->order.begin(); itI != cube->order.end(); ++itI)
			{
				if ((*itI) == index) (*itI) = 0;
				else if ((*itI) > index) --(*itI);
			}
		}

		SetNewChildFlag();
	}
}

// compare value
bool FCDEffectParameterSurface::IsValueEqual(FCDEffectParameter *parameter)
{
	if (!FCDEffectParameter::IsValueEqual(parameter)) return false;
	FCDEffectParameterSurface *param = (FCDEffectParameterSurface*)parameter;
	
	// compage images
	size_t imgCount = this->GetImageCount();
	if (imgCount != param->GetImageCount()) return false;
	
	for (size_t i = 0; i < imgCount; i++)
	{
		if (this->GetImage(i) != param->GetImage(i)) return false;
	}

	// compare initialisation methods
	if (initMethod != NULL && param->GetInitMethod() != NULL)
	{
		if (initMethod->GetInitType() != param->GetInitMethod()->GetInitType()) return false;
	}
	else
	{
		if (initMethod != param->GetInitMethod()) return false;
	}

	if (size != param->GetSize()) return false;
	if (mipLevelCount != param->GetMipLevelCount()) return false;
	if (generateMipmaps != param->IsGenerateMipMaps()) return false;
	if (viewportRatio != param->GetViewportRatio()) return false;

	// if you want to add more tests, feel free!

	return true;
}

// Clone
FCDEffectParameter* FCDEffectParameterSurface::Clone(FCDEffectParameter* _clone) const
{
	FCDEffectParameterSurface* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDEffectParameterSurface(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->GetObjectType() == FCDEffectParameterSurface::GetClassType()) clone = (FCDEffectParameterSurface*) _clone;

	if (_clone != NULL) FCDEffectParameter::Clone(_clone);
	if (clone != NULL)
	{
		clone->images.reserve(images.size());
		for (const FCDImage** itI = images.begin(); itI != images.end(); ++itI)
		{
			clone->AddImage(const_cast<FCDImage*>(*itI));
		}

		if (initMethod != NULL)
		{
			clone->initMethod = FCDEffectParameterSurfaceInitFactory::Create(initMethod->GetInitType());
			initMethod->Clone(clone->initMethod);
		}

		clone->size = size;
		clone->viewportRatio = viewportRatio;
		clone->mipLevelCount = mipLevelCount;
		clone->generateMipmaps = generateMipmaps;

		clone->format = format;
		if (formatHint != NULL)
		{
			FCDFormatHint* cloneHint = clone->AddFormatHint();
			cloneHint->channels = formatHint->channels;
			cloneHint->range = formatHint->range;
			cloneHint->precision = formatHint->precision;
			cloneHint->options = formatHint->options;
		}
	}
	return _clone;
}

// Flattening: overwrite the target parameter with this parameter
void FCDEffectParameterSurface::Overwrite(FCDEffectParameter* target)
{
	if (target->GetType() == SURFACE)
	{
		FCDEffectParameterSurface* s = (FCDEffectParameterSurface*) target;
		s->images.clear();
		for (uint32 i=0; i<images.size(); i++)
			s->images.push_back(images[i]);

//		s->initMethod->initType = initMethod->GetInitType();
		s->size = size;
		s->viewportRatio = viewportRatio;
		s->mipLevelCount = mipLevelCount;
		s->generateMipmaps = generateMipmaps;
		SetDirtyFlag();
	}
}

FCDFormatHint* FCDEffectParameterSurface::AddFormatHint()
{
	if (formatHint == NULL) formatHint = new FCDFormatHint();
	return formatHint;
}

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInit::Clone(FCDEffectParameterSurfaceInit* clone) const
{
	if (clone == NULL)
	{
		clone = FCDEffectParameterSurfaceInitFactory::Create(GetInitType());
		return clone != NULL ? Clone(clone) : NULL;
	}
	else
	{
		//no member variables to copy in this class, but leave this for future use.
		return clone;
	}
}

//
// FCDEffectParameterSurfaceInitCube
//

FCDEffectParameterSurfaceInitCube::FCDEffectParameterSurfaceInitCube()
{
	cubeType = ALL;
}

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitCube::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitCube* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitCube();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitCube*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		clone->cubeType = cubeType;
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitFrom
// 

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitFrom::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitFrom* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitFrom();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitFrom*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		clone->face = face;
		clone->mip = mip;
		clone->slice = slice;
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitVolume
//

FCDEffectParameterSurfaceInitVolume::FCDEffectParameterSurfaceInitVolume()
{
	volumeType = ALL;
}

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitVolume::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitVolume* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitVolume();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitVolume*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		clone->volumeType = volumeType;
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitAsNull
//

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitAsNull::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitAsNull* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitAsNull();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitAsNull*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		// Nothing to clone.
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitAsTarget
//

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitAsTarget::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitAsTarget* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitAsTarget();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitAsTarget*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		// Nothing to clone.
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitPlanar
//

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitPlanar::Clone(FCDEffectParameterSurfaceInit* _clone) const
{
	FCDEffectParameterSurfaceInitPlanar* clone = NULL;
	if (_clone == NULL) clone = new FCDEffectParameterSurfaceInitPlanar();
	else if (_clone->GetInitType() == GetInitType()) clone = (FCDEffectParameterSurfaceInitPlanar*) _clone;

	if (_clone != NULL) FCDEffectParameterSurfaceInit::Clone(_clone);
	if (clone != NULL)
	{
		// Nothing to clone.
	}
	return clone;
}

//
// FCDEffectParameterSurfaceInitFactory
//

FCDEffectParameterSurfaceInit* FCDEffectParameterSurfaceInitFactory::Create(InitType type)
{
	FCDEffectParameterSurfaceInit* parameter = NULL;

	switch (type)
	{
	case FCDEffectParameterSurfaceInitFactory::AS_NULL:		parameter = new FCDEffectParameterSurfaceInitAsNull(); break;
	case FCDEffectParameterSurfaceInitFactory::AS_TARGET:	parameter = new FCDEffectParameterSurfaceInitAsTarget(); break;
	case FCDEffectParameterSurfaceInitFactory::CUBE:		parameter = new FCDEffectParameterSurfaceInitCube(); break;
	case FCDEffectParameterSurfaceInitFactory::FROM:		parameter = new FCDEffectParameterSurfaceInitFrom(); break;
	case FCDEffectParameterSurfaceInitFactory::PLANAR:		parameter = new FCDEffectParameterSurfaceInitPlanar(); break;
	case FCDEffectParameterSurfaceInitFactory::VOLUME:		parameter = new FCDEffectParameterSurfaceInitVolume(); break;
	default: break;
	}

	return parameter;
}
