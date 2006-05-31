#ifndef MISCSTATE_H__
#define MISCSTATE_H__

#include "General/Observable.h"

namespace AtlasMessage
{
	typedef int ObjectID;
}

extern wxString g_SelectedTexture;

extern Observable<std::vector<AtlasMessage::ObjectID> > g_SelectedObjects;

#endif // MISCSTATE_H__
