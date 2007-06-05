#include "precompiled.h"

#include "MiscState.h"

wxString g_SelectedTexture = _T("grass1_spring");

Observable<std::vector<AtlasMessage::ObjectID> > g_SelectedObjects;
