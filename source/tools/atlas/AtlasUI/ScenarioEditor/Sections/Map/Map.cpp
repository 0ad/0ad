#include "stdafx.h"

#include "Map.h"

#include "ActionButton.h"

#include "GameInterface/Messages.h"

static void GenerateMap()
{
	ADD_COMMAND(GenerateMap(9));
}

MapSidebar::MapSidebar(wxWindow* parent)
	: Sidebar(parent)
{
	m_MainSizer->Add(new ActionButton(this, _T("Generate Map"), &GenerateMap));
}
