/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDocument.h"
#include "FCDocument/FCDLibrary.h"
#include "FCDocument/FCDAnimationClip.h"
#include "FCDocument/FCDAnimationClipTools.h"
#include "FCDocument/FCDAnimationCurve.h"

namespace FCDAnimationClipTools
{
	void ResetAnimationClipTimes(FCDocument* document, float startValue)
	{
		for (size_t i = 0; i < document->GetAnimationClipLibrary()->GetEntityCount(); i++)
		{
			FCDAnimationClip* clip = document->GetAnimationClipLibrary()->GetEntity(i);
			
			// Calculate the offset and apply it to the start/end on the clip.
			float offset = startValue - clip->GetStart();
			clip->SetStart(startValue);
			clip->SetEnd(clip->GetEnd() + offset);

			// Apply the offset on the curve keys.
			for (FCDAnimationCurveTrackList::iterator it = clip->GetClipCurves().begin(); it != clip->GetClipCurves().end(); ++it)
			{
				(*it)->SetClipOffset(offset, clip);
			}
		}
	}
};
