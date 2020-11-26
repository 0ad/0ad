/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUTracker.h"

//
// FUTrackable
//

ImplementObjectType(FUTrackable);

FUTrackable::FUTrackable()
{
}

FUTrackable::~FUTrackable()
{
	// Detach this object from its trackers.
	Detach();
}

void FUTrackable::Detach()
{
	for (FUTrackerList::iterator itT = trackers.begin(); itT != trackers.end(); ++itT)
	{
		(*itT)->OnObjectReleased(this);
	}
	trackers.clear();

	// Also detach from the owner.
	FUObject::Detach();
}

// Manage the list of trackers
void FUTrackable::AddTracker(FUTracker* tracker)
{
	FUAssert(!trackers.contains(tracker), return);
	trackers.push_back(tracker);
}
void FUTrackable::RemoveTracker(FUTracker* tracker)
{
	//FUAssert(trackers.contains(tracker), return);
	//trackers.erase(tracker);
	FUAssert(trackers.erase(tracker),);
}
bool FUTrackable::HasTracker(const FUTracker* tracker) const
{
	return trackers.contains(const_cast<FUTracker*>(tracker));
}
