/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDImage.h"
#include "FUtils/FUUri.h"
#include "FUtils/FUFileManager.h"

//
// FCDImage
//

ImplementObjectType(FCDImage);

FCDImage::FCDImage(FCDocument* document)
:	FCDEntity(document, "Image")
,	InitializeParameterNoArg(filename)
,	InitializeParameter(width, 0)
,	InitializeParameter(height, 0)
,	InitializeParameter(depth, 0)
{
}

FCDImage::~FCDImage()
{
}

void FCDImage::SetFilename(const fstring& _filename)
{
	ResetVideoFlag();
	if (_filename.empty()) filename->clear();
	else
	{
		filename = GetDocument()->GetFileManager()->GetCurrentUri().MakeAbsolute(_filename);
	}
	SetDirtyFlag();
}

// Copies the image entity into a clone.
FCDEntity* FCDImage::Clone(FCDEntity* _clone, bool cloneChildren) const
{
	FCDImage* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDImage(const_cast<FCDocument*>(GetDocument()));
	else if (_clone->HasType(FCDImage::GetClassType())) clone = (FCDImage*) _clone;

	FCDEntity::Clone(_clone, cloneChildren);

	if (clone != NULL)
	{
		clone->width = width;
		clone->height = height;
		clone->depth = depth;
		clone->filename = filename;
		clone->SetVideoFlag(GetVideoFlag());
	}
	return _clone;
}
