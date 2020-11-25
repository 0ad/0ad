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
#include "FCDocument/FCDEffect.h"
#include "FCDocument/FCDEffectCode.h"
#include "FCDocument/FCDEffectProfileFX.h"
#include "FCDocument/FCDEffectParameter.h"
#include "FCDocument/FCDEffectParameterFactory.h"
#include "FCDocument/FCDEffectTechnique.h"
#include "FCDocument/FCDExtra.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDImage.h"

//
// FCDEffectProfileFX
//

ImplementObjectType(FCDEffectProfileFX);
ImplementParameterObject(FCDEffectProfileFX, FCDEffectCode, codes, new FCDEffectCode(parent->GetDocument()));
ImplementParameterObject(FCDEffectProfileFX, FCDEffectTechnique, techniques, new FCDEffectTechnique(parent->GetDocument(), parent));

FCDEffectProfileFX::FCDEffectProfileFX(FCDocument* document, FCDEffect* _parent)
:	FCDEffectProfile(document, _parent)
,	InitializeParameter(type, FUDaeProfileType::UNKNOWN)
,	InitializeParameterNoArg(platform)
,	InitializeParameterNoArg(codes)
,	InitializeParameterNoArg(techniques)
{
}

FCDEffectProfileFX::~FCDEffectProfileFX()
{
}

FCDEffectTechnique* FCDEffectProfileFX::AddTechnique()
{
	FCDEffectTechnique* technique = new FCDEffectTechnique(GetDocument(), this);
	techniques.push_back(technique);
	SetNewChildFlag();
	return technique;
}

const FCDEffectCode* FCDEffectProfileFX::FindCode(const char* sid) const
{
	for (const FCDEffectCode** itC = codes.begin(); itC != codes.end(); ++itC)
	{
		if ((*itC)->GetSubId() == sid) return (*itC);
	}
	return NULL;
}

// Adds a new code inclusion to this effect profile.
FCDEffectCode* FCDEffectProfileFX::AddCode()
{
	FCDEffectCode* code = new FCDEffectCode(GetDocument());
	codes.push_back(code);
	SetNewChildFlag();
	return code;
}

// Clone the profile effect and its parameters
FCDEffectProfile* FCDEffectProfileFX::Clone(FCDEffectProfile* _clone) const
{
	FCDEffectProfileFX* clone = NULL;
	if (_clone == NULL) { _clone = clone = new FCDEffectProfileFX(const_cast<FCDocument*>(GetDocument()), const_cast<FCDEffect*>(GetParent())); }
	else if (_clone->GetObjectType() == FCDEffectProfileFX::GetClassType()) clone = (FCDEffectProfileFX*) _clone;

	if (_clone != NULL) FCDEffectProfile::Clone(_clone);
	if (clone != NULL)
	{
		clone->type = type;

		// Clone the codes: needs to happen before the techniques are cloned.
		clone->codes.reserve(codes.size());
		for (const FCDEffectCode** itC = codes.begin(); itC != codes.end(); ++itC)
		{
			FCDEffectCode* clonedCode = clone->AddCode();
			(*itC)->Clone(clonedCode);
		}

		// Clone the techniques
		clone->techniques.reserve(techniques.size());
		for (const FCDEffectTechnique** itT = techniques.begin(); itT != techniques.end(); ++itT)
		{
			FCDEffectTechnique* clonedTechnique = clone->AddTechnique();
            (*itT)->Clone(clonedTechnique);
		}
	}

	return _clone;
}
