#ifndef MISCSTATE_H__
#define MISCSTATE_H__

#include "General/Observable.h"

namespace AtlasMessage
{
	typedef int ObjectID;
}

extern wxString g_SelectedTexture;

// Observer order:
//  0 = g_UnitSettings
//  1 = things that want to access g_UnitSettings
extern Observable<std::vector<AtlasMessage::ObjectID> > g_SelectedObjects;

#endif // MISCSTATE_H__
